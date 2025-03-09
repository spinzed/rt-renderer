#include "renderer/Raytracer.cuh"

#include <objects/Plane.h>
#include <objects/Sphere.h>
#include <utils/Timer.h>

#include <cmath>
#include <cuda_runtime.h>
#include <curand.h>
#include <curand_kernel.h>
#include <iostream>

// debug
#define gpuErrchk(ans)                                                                                                 \
    {                                                                                                                  \
        gpuAssert((ans), __FILE__, __LINE__);                                                                          \
    }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort = true) {
    if (code != cudaSuccess) {
        fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        exit(1);
    }
}

// constants for raytracer
__constant__ int cwidth;
__constant__ int cheight;
__constant__ int cdepth;
__constant__ int crpp;
__constant__ RTRenderSettings csettings;

// additional operators for float3
__device__ float3 operator+(const float3 a, const float3 b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }

__device__ float3 operator-(const float3 a, const float3 b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }

__device__ float3 operator-(const float3 a) { return make_float3(-a.x, -a.y, -a.z); }

__device__ float3 operator*(const float3 a, const float b) { return make_float3(a.x * b, a.y * b, a.z * b); }

__device__ float3 operator*(const float b, const float3 a) { return make_float3(a.x * b, a.y * b, a.z * b); }

__device__ float3 operator*(const float3 a, const float3 b) { return make_float3(a.x * b.x, a.y * b.y, a.z * b.z); }

__device__ float3 operator/(const float3 a, const float b) { return make_float3(a.x / b, a.y / b, a.z / b); }

// for some reason dot is not defined
__device__ __host__ inline float dot(const float3 &a, const float3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// additional utility functions
__device__ float3 reflect(const float3 &I, const float3 &N) { return I - 2.0f * dot(I, N) * N; }

__device__ inline float3 normalize(float3 v) { return v / sqrt(dot(v, v)); }

__device__ float3 cross(const float3 a, const float3 b) {
    return make_float3(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y);
}

__device__ float3 lerp(const float3 a, const float3 b, const float t) { return a * (1.0f - t) + b * t; }

inline float3 vec3_to_float3(glm::vec3 v) { return make_float3(v.x, v.y, v.z); }

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_E
#define M_E 3.14159265358979323846f
#endif

__device__ float3 randomPointOnHemisphere(float3 normal, curandState *randState) {
    // Generate two random numbers in [0,1]
    float3 point = make_float3(curand_normal(randState), curand_normal(randState), curand_normal(randState));
    if (dot(point, normal) < 0) {
        return -point;
    }
    return point;
}

__device__ float2 randomPointInCircle(curandState *randState) {
    float angle = 2.0f * M_PI * curand_uniform(randState);
    float radius = sqrtf(curand_uniform(randState));
    return make_float2(radius * cosf(angle), radius * sinf(angle));
}

// intersection detection
__device__ bool intersectSphere(const RTRay &ray, const RTSphere &sphere, float &t) {
    float3 oc = ray.origin - sphere.center;

    float a = dot(ray.direction, ray.direction);
    float b = 2.0f * dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        return false; // No intersection

    float sqrtD = sqrtf(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    if (t1 > 0.0f) {
        t = t1; // Closest valid intersection
        return true;
    } else if (t2 > 0.0f) {
        t = t2;
        return true;
    }

    return false; // behind the ray
}

__device__ int raycastSphere(const RTRay &ray, const RTSphere *spheres, int numSpheres, float &t, float3 &hitPoint,
                             float3 &normal) {
    if (!csettings.renderSpheres)
        return -1;

    int closestSphereIdx = -1;
    float closestT = 1e30f;

    for (int i = 0; i < numSpheres; i++) {
        float t;
        if (intersectSphere(ray, spheres[i], t) && t < closestT) {
            closestT = t;
            closestSphereIdx = i;
        }
    }
    if (closestSphereIdx != -1) {
        t = closestT;
        hitPoint = ray.origin + ray.direction * closestT;
        normal = normalize(hitPoint - spheres[closestSphereIdx].center);
    }

    return closestSphereIdx;
}

__device__ bool intersectPlane(const RTRay &ray, const RTPlane &plane, float &t) {
    float denom = dot(ray.direction, plane.normal);

    // if (fabsf(denom) < 1e-6f)
    //     return false;
    if (denom > -1e-6f) // don't render the backsides
        return false;

    float3 diff = plane.center - ray.origin;
    t = dot(diff, plane.normal) / denom;

    if (t < 0.0f)
        return false;
    // this is computed again later, can be avoided
    float3 hit_point = ray.origin + t * ray.direction;

    // convert hit_point to plane's local coordinate system
    float3 local = hit_point - plane.center;
    // return fabs(local.x) <= plane.width && fabs(local.z) <= plane.height;

    float u_proj = dot(local, plane.u);
    float v_proj = dot(local, plane.v);

    return fabsf(u_proj) <= plane.height && fabsf(v_proj) <= plane.width;
}

__device__ int raycastPlane(const RTRay &ray, const RTPlane *planes, const int numPlanes, float &t, float3 &hitPoint,
                            float3 &normal) {
    if (!csettings.renderPlanes)
        return -1;

    int closestIndex = -1;
    float closestT = 1e30f;

    for (int i = 0; i < numPlanes; i++) {
        float t;
        if (intersectPlane(ray, planes[i], t) && t < closestT) {
            closestT = t;
            closestIndex = i;
        }
    }
    if (closestIndex != -1) {
        t = closestT;
        hitPoint = ray.origin + ray.direction * closestT;
        normal = planes[closestIndex].normal;
    }

    return closestIndex;
}

__device__ bool intersectTriangle(const RTRay &ray, float3 v0, float3 v1, float3 v2, float &t, float3 &normal) {
    float3 brid1 = v1 - v0;
    float3 brid2 = v2 - v0;

    normal = cross(brid1, brid2);
    float det = -dot(ray.direction, normal);

    if (det < 1e-6)
        return false;

    float invdet = 1.0f / det;

    float3 AO = ray.origin - v0;
    float3 DAO = cross(AO, ray.direction);

    float u = dot(brid2, DAO) * invdet;
    float v = -dot(brid1, DAO) * invdet;
    t = dot(AO, normal) * invdet;
    return t >= 0.0f && u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f;
}

__device__ float3 getVertex(float *vertices, int *indices, int index) {
    return make_float3(vertices[3 * indices[index]], vertices[3 * indices[index] + 1],
                       vertices[3 * indices[index] + 2]);
}

__device__ bool intersectMesh(const RTRay &ray, const RTMesh &mesh, float *vertices, int *indices, float &t,
                              float3 &normal) {
    for (int i = 0; i < mesh.indexNumber / 3; i++) {
        float3 v0 = getVertex(vertices, indices, mesh.indexOffset + 3 * i);
        float3 v1 = getVertex(vertices, indices, mesh.indexOffset + 3 * i + 1);
        float3 v2 = getVertex(vertices, indices, mesh.indexOffset + 3 * i + 2);
        if (intersectTriangle(ray, v0, v1, v2, t, normal)) {
            return true;
        }
    }
    return false;
}

__device__ int raycastMesh(const RTRay &ray, const RTMesh *meshes, int numMeshes, float *vertices, int *indices,
                           float &t, float3 &hitPoint, float3 &normal) {
    if (!csettings.renderMeshes)
        return -1;

    int closestIndex = -1;
    float closestT = 1e30f;

    for (int i = 0; i < numMeshes; i++) {
        float t;
        if (intersectMesh(ray, meshes[i], vertices, indices, t, normal) && t < closestT) {
            closestT = t;
            closestIndex = i;
        }
    }
    if (closestIndex != -1) {
        t = closestT;
        hitPoint = ray.origin + ray.direction * closestT;
        // normal is set already
    }

    return closestIndex;
}

__device__ bool raycast(const RTScene *scene, const RTRay &ray, float3 &point, float3 &normal, RTMaterial &material) {
    float t, t1;
    int materialIndex = -1;
    bool hit = false;
    float3 tempHitpoint, tempNormal;

    int indexS = raycastSphere(ray, scene->spheres, scene->sphereNum, t, point, normal);
    if (indexS >= 0) {
        hit = true;
        materialIndex = scene->spheres[indexS].materialIndex;
    }
    int indexP = raycastPlane(ray, scene->planes, scene->planeNum, t1, tempHitpoint, tempNormal);
    if (indexP >= 0 && (!hit || t1 < t)) {
        hit = true;
        t = t1;
        point = tempHitpoint;
        normal = tempNormal;
        materialIndex = scene->planes[indexP].materialIndex;
    }
    int indexM =
        raycastMesh(ray, scene->meshes, scene->meshNum, scene->vertices, scene->indices, t1, tempHitpoint, tempNormal);
    if (indexM >= 0 && (!hit || t1 < t)) {
        hit = true;
        t = t1;
        point = tempHitpoint;
        normal = tempNormal;
        materialIndex = scene->meshes[indexM].materialIndex;
    }
    if (materialIndex != -1)
        material = scene->materials[materialIndex];

    return indexS >= 0 || indexP >= 0 || indexM >= 0;
}

__device__ float3 raytrace(RTScene *scene, const RTRay &ray, int depth, curandState &randState,
                           float3 rayColor = make_float3(1, 1, 1)) {
    if (depth <= 0)
        return make_float3(0, 0, 0);

    float3 hitPoint, normal, color;
    RTMaterial material;
    if (!raycast(scene, ray, hitPoint, normal, material))
        return make_float3(0.2, 0.2, 0.2);

    float3 reflected = normalize(reflect(ray.direction, normal));
    float3 dispersed = normalize(normal + randomPointOnHemisphere(normal, &randState));

    float3 newNormal = lerp(dispersed, reflected, material.smoothness);

    RTRay newRay = {.origin = hitPoint, .direction = newNormal};
    // prevent hitting the same thing
    newRay.origin = newRay.origin + 0.001f * newRay.direction;

    float3 newColor = rayColor * material.emission * material.emissionStrength;
    rayColor = rayColor * material.diffuse;
    return newColor + raytrace(scene, newRay, depth - 1, randState, rayColor);
}

__global__ void calculatePixel(RTScene *scene, float *output, unsigned long long seed, curandState *states) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (!(x < cwidth && y < cheight))
        return;

    int pixNum = y * cwidth + x;
    curand_init((seed << 20) + pixNum, 0, 0, &states[pixNum]);

    float3 color = make_float3(0, 0, 0);

    float3 focalPoint = scene->camera.topLeft + x * scene->camera.dx + (cheight - y) * scene->camera.dy;
    float3 cameraToFp = focalPoint - scene->camera.ray.origin;
    float3 originAdjustment = make_float3(0.0f, 0.0f, 0.0f);
    float3 fpAdjustment = make_float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < crpp; i++) {
        if (csettings.dofStrength > 0.0f) {
            float2 point = randomPointInCircle(&states[pixNum]);
            originAdjustment = csettings.dofStrength * (point.x * scene->camera.dx + point.y * scene->camera.dy);
        }
        if (csettings.blurriness > 0.0f) {
            float2 point = randomPointInCircle(&states[pixNum]);
            fpAdjustment = csettings.blurriness * (point.x * scene->camera.dx + point.y * scene->camera.dy);
        }
        float3 finalOrigin = scene->camera.ray.origin + originAdjustment;
        float3 finalFocalPoint = scene->camera.ray.origin + cameraToFp * csettings.dofDistance + fpAdjustment;
        RTRay ray = {
            .origin = finalOrigin,
            .direction = normalize(finalFocalPoint - finalOrigin),
        };
        color = color + raytrace(scene, ray, cdepth, states[pixNum]);
    }

    color = color / crpp;

    int firstComp = 3 * pixNum;
    output[firstComp] = color.x;
    output[firstComp + 1] = color.y;
    output[firstComp + 2] = color.z;
}

__global__ void monteCarloGpu(int width, int height, int renderCount, float *input1, float *input2) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (!(x < cwidth && y < cheight))
        return;

    int pixNum = y * cwidth + x;

    int rasterPos = 3 * pixNum;

    float3 originalColor1 = make_float3(input1[rasterPos], input1[rasterPos + 1], input1[rasterPos + 2]);
    float3 originalColor2 = make_float3(input2[rasterPos], input2[rasterPos + 1], input2[rasterPos + 2]);
    float3 color = (originalColor1 * (renderCount - 1) + originalColor2) / (renderCount);

    input1[rasterPos] = color.x;
    input1[rasterPos + 1] = color.y;
    input1[rasterPos + 2] = color.z;
}

// exposed functions

namespace CudaRT {

void init() { std::srand((unsigned int)std::time({})); }

void render(int width, int height, int depth, int rpp, RenderData data, float *output) {
    // allocate memory for the output raster on the gpu
    Timer t = Timer::start();
    debugString = "";

    dim3 blockSize(gridSize, gridSize); // 16x16 = 256 threads per block
    dim3 gridSizeFinal((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);

    if (gridSizeFinal.x >= 32768 || gridSizeFinal.y >= 32768)
        return;

    cudaDeviceSetLimit(cudaLimitStackSize, 8192); // Try 8KB, increase if needed

    // constants
    cudaMemcpyToSymbol(cwidth, &width, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(cheight, &height, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(cdepth, &depth, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(crpp, &rpp, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(csettings, &settings, sizeof(RTRenderSettings), 0, cudaMemcpyHostToDevice);

    // precompute screen positions in world space
    Camera *camera = data.camera;
    glm::vec3 camPos = camera->position();
    CameraConstraints c = camera->constraints;
    glm::vec3 start = camPos + c.nearPlane * camera->forward() + c.top * camera->up() + c.left * camera->right();
    glm::vec3 row = (c.right - c.left) * camera->right();
    glm::vec3 dx = row * (1.0f / width);
    glm::vec3 column = -(c.top - c.bottom) * camera->up();
    glm::vec3 dy = column * (1.0f / height);

    // transform scene data into format fit for transfer to vram
    std::vector<RTMaterial> materials;
    std::vector<RTSphere> spheres;
    std::vector<RTPlane> planes;
    std::vector<RTMesh> meshes;
    std::vector<float> vertices;
    std::vector<int> indices;

    for (Object *obj : *data.objects) {
        if (obj->type == "sphere") {
            Sphere *s = (Sphere *)obj;
            Transform tr = obj->getFullTransform();
            materials.push_back(RTMaterial{
                .emission = obj->material ? vec3_to_float3(obj->material->colorEmissive) : make_float3(0, 0, 0),
                .emissionStrength = obj->material ? obj->material->emissiveStrength : 0.0f,
                .diffuse = vec3_to_float3(s->color),
                .smoothness = obj->material ? obj->material->smoothness : 0.0f,
            });
            spheres.push_back(RTSphere{
                .center = vec3_to_float3(tr.position()),
                .radius = tr.getScale().x,
                .materialIndex = (int)materials.size() - 1,
            });
        } else if (obj->type == "plane") {
            Plane *p = (Plane *)obj;
            glm::vec3 dims = p->getTransform()->getScale();
            glm::vec3 u, v;
            p->uv(u, v);
            materials.push_back(RTMaterial{
                .emission = obj->material ? vec3_to_float3(obj->material->colorEmissive) : make_float3(0, 0, 0),
                .emissionStrength = obj->material ? obj->material->emissiveStrength : 0.0f,
                .diffuse = vec3_to_float3(p->color),
                .smoothness = obj->material ? obj->material->smoothness : 0.0f,
            });
            planes.push_back(RTPlane{
                .center = vec3_to_float3(p->center()),
                .normal = vec3_to_float3(p->normal()),
                .width = dims.x,
                .height = dims.y,
                .u = vec3_to_float3(u),
                .v = vec3_to_float3(v),
                .materialIndex = (int)materials.size() - 1,
            });
        } else if (obj->type == "mesh") {
            MeshObject *m = (MeshObject *)obj;
            Mesh meshCopy = *m->mesh;
            meshCopy.applyTransform(m->getFullTransform().getMatrix());

            materials.push_back(RTMaterial{
                .emission = obj->material ? vec3_to_float3(obj->material->colorEmissive) : make_float3(0, 0, 0),
                .emissionStrength = obj->material ? obj->material->emissiveStrength : 0.0f,
                .diffuse = obj->material ? vec3_to_float3(obj->material->colorDiffuse) : make_float3(0, 0, 0),
                .smoothness = obj->material ? obj->material->smoothness : 0.0f,
            });
            int indexOffset = (int)indices.size();

            vertices.insert(vertices.end(), meshCopy.vrhovi.begin(), meshCopy.vrhovi.end());
            indices.insert(indices.end(), m->mesh->indeksi.begin(), m->mesh->indeksi.end());

            meshes.push_back(RTMesh{
                .indexOffset = indexOffset,
                .indexNumber = (int)indices.size() - indexOffset,
                .materialIndex = (int)materials.size() - 1,
            });
        }
    }

    // for (const RTPlane& plane: planes) {
    //     std::cout << plane.normal.x << " " << plane.normal.y << " " << plane.normal.z << std::endl;
    // }

    RTScene h_scene = {
        .camera =
            {
                .ray =
                    {
                        .origin = vec3_to_float3(data.camera->position()),
                        .direction = vec3_to_float3(data.camera->forward()),
                    },
                .topLeft = vec3_to_float3(start),
                .dx = vec3_to_float3(dx),
                .dy = vec3_to_float3(dy),
            },
        .sphereNum = (int)spheres.size(),
        .spheres = nullptr, // will point to d_spheres
        .planeNum = (int)planes.size(),
        .planes = nullptr, // will point to d_planes
        .meshNum = (int)meshes.size(),
        .meshes = nullptr,    // will point to d_meshes
        .vertices = nullptr,  // will point to d_vertices
        .indices = nullptr,   // will point do d_indices
        .materials = nullptr, // will point to d_materials
    };
    debugString += t.format("Object serialization: # ($)\n");

    RTScene *d_scene;

    float *d_output;
    int rasterSize = width * height * 3 * sizeof(float);
    gpuErrchk(cudaMalloc(&d_output, rasterSize));

    curandState *d_rand_states;
    gpuErrchk(cudaMalloc(&d_rand_states, width * height * sizeof(curandState)));

    // allocate GPU memory for scene parts
    gpuErrchk(cudaMalloc(&h_scene.spheres, h_scene.sphereNum * sizeof(RTSphere)));
    gpuErrchk(cudaMalloc(&h_scene.planes, h_scene.planeNum * sizeof(RTPlane)));
    gpuErrchk(cudaMalloc(&h_scene.meshes, h_scene.meshNum * sizeof(RTMesh)));
    gpuErrchk(cudaMalloc(&h_scene.vertices, vertices.size() * sizeof(float)));
    gpuErrchk(cudaMalloc(&h_scene.indices, indices.size() * sizeof(int)));
    gpuErrchk(cudaMalloc(&h_scene.materials, materials.size() * sizeof(RTMaterial)));

    // copy scene parts to vram
    gpuErrchk(
        cudaMemcpy(h_scene.spheres, spheres.data(), h_scene.sphereNum * sizeof(RTSphere), cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(h_scene.planes, planes.data(), h_scene.planeNum * sizeof(RTPlane), cudaMemcpyHostToDevice));
    gpuErrchk(
        cudaMemcpy(h_scene.materials, materials.data(), materials.size() * sizeof(RTMaterial), cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(h_scene.meshes, meshes.data(), h_scene.meshNum * sizeof(RTMesh), cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(h_scene.vertices, vertices.data(), vertices.size() * sizeof(float), cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(h_scene.indices, indices.data(), indices.size() * sizeof(int), cudaMemcpyHostToDevice));

    // allocate Scene struct on GPU and copy it
    gpuErrchk(cudaMalloc(&d_scene, sizeof(RTScene)));
    gpuErrchk(cudaMemcpy(d_scene, &h_scene, sizeof(RTScene), cudaMemcpyHostToDevice));

    debugString += t.format("Device malloc and copy: # ($)\n");
    int seed = std::rand();

    calculatePixel<<<gridSizeFinal, blockSize>>>(d_scene, d_output, seed, d_rand_states);
    cudaDeviceSynchronize();
    debugString += t.format("Rendering: # ($)\n");

    cudaMemcpy(output, d_output, rasterSize, cudaMemcpyDeviceToHost);

    gpuErrchk(cudaFree(h_scene.materials));
    gpuErrchk(cudaFree(h_scene.indices));
    gpuErrchk(cudaFree(h_scene.vertices));
    gpuErrchk(cudaFree(h_scene.meshes));
    gpuErrchk(cudaFree(h_scene.planes));
    gpuErrchk(cudaFree(h_scene.spheres));
    gpuErrchk(cudaFree(d_rand_states));
    gpuErrchk(cudaFree(d_scene));
    gpuErrchk(cudaFree(d_output));

    debugString += t.format("Copying the result and freeing device memory: # ($)\n");
}

void monteCarlo(int width, int height, int renderCount, float *input1, float *input2) {
    float *d_input1, *d_input2;
    int rasterSize = width * height * 3 * sizeof(float);

    dim3 blockSize(16, 16); // 16x16 = 256 threads per block
    dim3 gridSizeFinal((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);
    if (gridSizeFinal.x >= 32768 || gridSizeFinal.y >= 32768)
        return;

    gpuErrchk(cudaMalloc(&d_input1, rasterSize));
    gpuErrchk(cudaMalloc(&d_input2, rasterSize));

    gpuErrchk(cudaMemcpy(d_input1, input1, rasterSize, cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(d_input2, input2, rasterSize, cudaMemcpyHostToDevice));

    monteCarloGpu<<<gridSizeFinal, blockSize>>>(width, height, renderCount, d_input1, d_input2);
    cudaDeviceSynchronize();

    gpuErrchk(cudaMemcpy(input1, d_input1, rasterSize, cudaMemcpyDeviceToHost));

    gpuErrchk(cudaFree(d_input1));
    gpuErrchk(cudaFree(d_input2));
}

} // namespace CudaRT
