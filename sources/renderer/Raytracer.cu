#include "renderer/Raytracer.cuh"

#include <objects/Plane.h>
#include <objects/Sphere.h>
#include <utils/Timer.h>

#include <cmath>
#include <cuda_runtime.h>
#include <curand.h>
#include <curand_kernel.h>
#include <iostream>

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

    if (fabsf(denom) < 1e-6f)
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

__device__ bool raycast(const RTScene *scene, const RTRay &ray, float3 &point, float3 &normal, RTMaterial &material) {
    float t, t1;
    int materialIndex = -1;
    float3 tempHitpoint, tempNormal;

    int indexS = raycastSphere(ray, scene->spheres, scene->sphereNum, t, point, normal);
    if (indexS >= 0) {
        materialIndex = scene->spheres[indexS].materialIndex;
    }
    int indexP = raycastPlane(ray, scene->planes, scene->planeNum, t1, tempHitpoint, tempNormal);
    if (indexP >= 0 && (indexS < 0 || t1 < t)) {
        point = tempHitpoint;
        normal = tempNormal;
        materialIndex = scene->planes[indexP].materialIndex;
    }
    if (materialIndex != -1)
        material = scene->materials[materialIndex];

    return indexS >= 0 || indexP >= 0;
}

__device__ float3 raytrace(RTScene *scene, const RTRay &ray, int depth, curandState &randState,
                           float3 rayColor = make_float3(1, 1, 1)) {
    if (depth <= 0)
        return make_float3(0, 0, 0);

    float3 hitPoint, normal, color;
    RTMaterial material;
    if (!raycast(scene, ray, hitPoint, normal, material))
        return make_float3(0.0, 0.0, 0.0);

    float3 reflected = normalize(reflect(ray.direction, normal));
    float3 dispersed = randomPointOnHemisphere(normal, &randState);

    // float3 newNormal = normalize(0.8f * reflected + 0.2f * dispersed);
    float3 newNormal = dispersed;

    RTRay newRay = {.origin = hitPoint, .direction = newNormal};
    // prevent hitting the same thing
    newRay.origin = newRay.origin + 0.001f * newRay.direction;

    float3 newColor = rayColor * material.emission * material.emissionStrength;
    rayColor = rayColor * material.diffuse;
    // newColor = depth == 2 ? make_float3(1, 0, 0) : make_float3(0, 0, 1);
    return newColor + raytrace(scene, newRay, depth - 1, randState, rayColor);
}

__constant__ int cwidth;
__constant__ int cheight;
__constant__ int cdepth;

__global__ void calculatePixel(RTScene *scene, float *output, unsigned long long seed, curandState *states) {
    int pixNum = blockIdx.x * blockDim.x + threadIdx.x;
    int x = pixNum % cwidth;
    int y = pixNum / cheight;

    if (!(x < cwidth && y < cheight))
        return;

    float3 pos = scene->camera.topLeft + x * scene->camera.dx + (cheight - y) * scene->camera.dy;
    RTRay ray = {
        .origin = scene->camera.ray.origin,
        .direction = normalize(pos - scene->camera.ray.origin),
    };

    curand_init((seed << 20) + pixNum, 0, 0, &states[pixNum]);

    float3 color = raytrace(scene, ray, cdepth, states[pixNum]);

    int firstComp = 3 * pixNum;
    output[firstComp] = color.x;
    output[firstComp + 1] = color.y;
    output[firstComp + 2] = color.z;
}

__global__ void monteCarloGpu(int width, int height, int renderCount, float *input1, float *input2) {
    int pixNum = blockIdx.x * blockDim.x + threadIdx.x;
    int x = pixNum % cwidth;
    int y = pixNum / cheight;

    if (!(x < width && y < height))
        return;
    if (renderCount < 1)
        return;

    int rasterPos = 3 * pixNum;

    float3 originalColor1 = make_float3(input1[rasterPos], input1[rasterPos + 1], input1[rasterPos + 2]);
    float3 originalColor2 = make_float3(input2[rasterPos], input2[rasterPos + 1], input2[rasterPos + 2]);
    float3 color = (originalColor1 * (renderCount - 1) + originalColor2) / renderCount;

    input1[rasterPos] = color.x;
    input1[rasterPos + 1] = color.y;
    input1[rasterPos + 2] = color.z;
}

// exposed functions

namespace CudaRT {

void init() { std::srand((unsigned int)std::time({})); }

void render(int width, int height, int depth, RenderData data, float *output) {
    // allocate memory for the output raster on the gpu
    Timer t = Timer::start();
    debugString = "";

    // size_t stackSize;
    // cudaDeviceGetLimit(&stackSize, cudaLimitStackSize);
    // debugString += std::format("CUDA Stack Size: {}\n", static_cast<std::uint64_t>(stackSize));

    cudaDeviceSetLimit(cudaLimitStackSize, 8192); // Try 8KB, increase if needed

    // constants
    cudaMemcpyToSymbol(cwidth, &width, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(cheight, &height, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(cdepth, &depth, sizeof(int), 0, cudaMemcpyHostToDevice);

    // precompute screen positions in world space
    Camera *camera = data.camera;
    glm::vec3 camPos = camera->position();
    CameraConstraints c = camera->constraints;
    glm::vec3 start = camPos + c.nearPlane * camera->forward() + c.top * camera->up() + c.left * camera->right();
    glm::vec3 row = (c.right - c.left) * camera->right();
    glm::vec3 dx = row * (1.0f / width);
    glm::vec3 column = -(c.top - c.bottom) * camera->up();
    glm::vec3 dy = column * (1.0f / height);

    int grid = 1024;
    int block = width * height / grid + 1;

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
            });
            spheres.push_back(RTSphere{
                .center = vec3_to_float3(tr.position()),
                .radius = tr.getScale().x,
                .materialIndex = (int)materials.size() - 1,
            });
        }
        if (obj->type == "plane") {
            Plane *p = (Plane *)obj;
            glm::vec3 dims = p->getTransform()->getScale();
            glm::vec3 u, v;
            p->uv(u, v);
            materials.push_back(RTMaterial{
                .emission = obj->material ? vec3_to_float3(obj->material->colorEmissive) : make_float3(0, 0, 0),
                .emissionStrength = obj->material ? obj->material->emissiveStrength : 0.0f,
                .diffuse = vec3_to_float3(p->color),
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
        }
    }

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
        .planes = nullptr,    // will point to d_planes
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
    gpuErrchk(cudaMalloc(&h_scene.materials, materials.size() * sizeof(RTMaterial)));

    // copy scene parts to vram
    gpuErrchk(
        cudaMemcpy(h_scene.spheres, spheres.data(), h_scene.sphereNum * sizeof(RTSphere), cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(h_scene.planes, planes.data(), h_scene.planeNum * sizeof(RTPlane), cudaMemcpyHostToDevice));
    gpuErrchk(
        cudaMemcpy(h_scene.materials, materials.data(), materials.size() * sizeof(RTMaterial), cudaMemcpyHostToDevice));

    // allocate Scene struct on GPU and copy it
    gpuErrchk(cudaMalloc(&d_scene, sizeof(RTScene)));
    gpuErrchk(cudaMemcpy(d_scene, &h_scene, sizeof(RTScene), cudaMemcpyHostToDevice));

    debugString += t.format("Device malloc and copy: # ($)\n");
    int seed = std::rand();

    calculatePixel<<<grid, block>>>(d_scene, d_output, seed, d_rand_states);
    cudaDeviceSynchronize();
    debugString += t.format("Rendering: # ($)\n");

    cudaMemcpy(output, d_output, rasterSize, cudaMemcpyDeviceToHost);

    gpuErrchk(cudaFree(h_scene.materials));
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
    gpuErrchk(cudaMalloc(&d_input1, rasterSize));
    gpuErrchk(cudaMalloc(&d_input2, rasterSize));

    gpuErrchk(cudaMemcpy(d_input1, input1, rasterSize, cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(d_input2, input2, rasterSize, cudaMemcpyHostToDevice));

    int grid = 1024;
    int block = width * height / grid + 1;

    monteCarloGpu<<<grid, block>>>(width, height, renderCount, d_input1, d_input2);
    cudaDeviceSynchronize();
    input1[33] = 0;
    input1[34] = 1;
    input1[35] = 0;

    gpuErrchk(cudaMemcpy(input1, d_input1, rasterSize, cudaMemcpyDeviceToHost));
    input1[30] = 1;
    input1[31] = 0;
    input1[32] = 0;

    gpuErrchk(cudaFree(d_input1));
    gpuErrchk(cudaFree(d_input2));
}

} // namespace CudaRT
