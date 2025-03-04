#include "renderer/Raytracer.cuh"

#include <cmath>
#include <cuda_runtime.h>
#include <iostream>
#include <objects/Plane.h>
#include <objects/Sphere.h>
#include <utils/Timer.h>

// additional operators for float3
__device__ float3 operator+(const float3 &a, const float3 &b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }

__device__ float3 operator-(const float3 &a, const float3 &b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }

__device__ float3 operator-(const float3 &a) { return make_float3(-a.x, -a.y, -a.z); }

__device__ float3 operator*(const float3 &a, const int &b) { return make_float3(a.x * b, a.y * b, a.z * b); }

__device__ float3 operator*(const int &b, const float3 &a) { return make_float3(a.x * b, a.y * b, a.z * b); }

__device__ float3 operator/(const float3 &a, const float &b) { return make_float3(a.x / b, a.y / b, a.z / b); }

// for some reason dot is not defined
__device__ __host__ inline float dot(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// additional utility functions
__device__ float3 reflect(const float3 &I, const float3 &N) { return I - 2.0f * dot(I, N) * N; }

__device__ inline float3 normalize(float3 v) { return v / sqrt(v.x * v.x + v.y + v.y + v.z * v.z); }

inline float3 vec3_to_float3(glm::vec3 v) { return make_float3(v.x, v.y, v.z); }

// intersection detection
__device__ bool intersectSphere(const RTRay &ray, const RTSphere &sphere, float &t) {
    float3 oc =
        make_float3(ray.origin.x - sphere.center.x, ray.origin.y - sphere.center.y, ray.origin.z - sphere.center.z);

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

    return false; // Intersection is behind the ray
}

__device__ int raycastSphere(const RTRay &ray, const RTSphere *spheres, const int numSpheres, float &t,
                             float3 &hitPoint, float3 &normal, float3 &color) {
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
        color = spheres[closestSphereIdx].color;
    }

    return closestSphereIdx;
}

__device__ bool intersectPlane(const RTRay &ray, const RTPlane plane, float &t) {
    float denom = dot(ray.direction, plane.normal);

    if (fabs(denom) < 1e-6f)
        return false; // Avoid division by near-zero

    float3 diff = plane.center - ray.origin;
    t = dot(diff, plane.normal) / denom;

    if (t <= 0.0f)
        return false; // behind the ray

    return true;

    // this is computed again later, can be avoided
    float3 hit_point = ray.origin + t * ray.direction;

    // Convert hit_point to plane's local coordinate system
    float3 local = hit_point - plane.center;
    float u_proj = dot(local, plane.u);
    float v_proj = dot(local, plane.v);

    // Check if inside rectangle bounds
    return fabs(u_proj) <= (plane.width * 0.5f) && fabs(v_proj) <= (plane.height * 0.5f);
}

__device__ int raycastPlane(const RTRay &ray, const RTPlane *planes, const int numPlanes, float &t, float3 &hitPoint,
                            float3 &normal, float3 &color) {
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
        color = planes[closestIndex].color;
    }

    return closestIndex;
}

__device__ bool raycast(const RTScene *scene, const RTRay &ray, float3 &point, float3 &normal, float3 &color) {
    float t, t1;
    float3 tempHitpoint, tempNormal, tempColor;
    int indexS = raycastSphere(ray, scene->spheres, scene->sphereNum, t, point, normal, color);
    int indexP = raycastPlane(ray, scene->planes, scene->planeNum, t1, tempHitpoint, tempNormal, tempColor);
    if (indexP >= 0 && t1 < t) {
        t = t1;
        point = tempHitpoint;
        normal = tempNormal;
        color = tempColor;
    }
    return indexS >= 0 || indexP >= 0;
}

__device__ float3 raytrace(RTScene *scene, RTRay ray, int depth) {
    if (depth == 0)
        return make_float3(0, 0, 1);

    float3 hitPoint, normal, color;
    if (!raycast(scene, ray, hitPoint, normal, color))
        return make_float3(0, 1, 0);

    RTRay newRay = {
        .origin = hitPoint,
        .direction = reflect(-ray.direction, normal),
    };
    // return color + raytrace(scene, newRay, depth - 1);
    return color;
}

__constant__ int cwidth;
__constant__ int cheight;
__constant__ int cdepth;

__global__ void calculatePixel(RTScene *scene, float *output) {
    int x = blockIdx.x;
    int y = threadIdx.x;

    if (!(x < cwidth && y < cheight))
        return;

    float3 pos = scene->camera.topLeft + x * scene->camera.dx + (cheight - y) * scene->camera.dy;
    RTRay ray = {
        .origin = scene->camera.ray.origin,
        .direction = pos - scene->camera.ray.origin,
    };

    float3 color = raytrace(scene, ray, cdepth);

    int firstComp = 3 * (y * cwidth + x);
    output[firstComp] = color.x;
    output[firstComp + 1] = color.y;
    output[firstComp + 2] = color.z;
}

// exposed functions

namespace CudaRT {

void render(int width, int height, RenderData data, float *output) {
    // allocate memory for the output raster on the gpu
    Timer t = Timer::start();
    debugString = "Started rendering\n";

    int depth = 3;

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
    int block = 1024;

    // transform scene data into format fit for transfer to vram
    std::vector<RTSphere> spheres;
    std::vector<RTPlane> planes;
    for (Object *obj : *data.objects) {
        if (obj->type == "sphere") {
            Sphere *s = (Sphere *)obj;
            Transform tr = obj->getFullTransform();
            spheres.push_back({.center = vec3_to_float3(tr.position()),
                               .radius = tr.getScale().x,
                               .color = vec3_to_float3(s->color)});
        }
        if (obj->type == "plane") {
            Plane *p = (Plane *)obj;
            glm::vec3 dims = p->getTransform()->getScale();
            glm::vec3 u, v;
            p->uv(u, v);
            planes.push_back({.center = vec3_to_float3(p->center()),
                              .normal = vec3_to_float3(p->normal()),
                              .width = dims.x,
                              .height = dims.y,
                              .u = vec3_to_float3(u),
                              .v = vec3_to_float3(v),
                              .color = vec3_to_float3(p->color)});
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
        .planes = nullptr, // will point to d_planes
    };
    debugString += t.format("Object serialization: # ($)\n");

    RTScene *d_scene;

    float *d_output;
    int rasterSize = width * height * 3 * sizeof(float);
    cudaMalloc(&d_output, rasterSize);

    // allocate GPU memory for scene parts
    cudaMalloc(&h_scene.spheres, h_scene.sphereNum * sizeof(RTSphere));
    cudaMalloc(&h_scene.planes, h_scene.planeNum * sizeof(RTPlane));
    // cudaMalloc(&h_scene.vertices, numVertices * sizeof(float3));
    // cudaMalloc(&h_scene.triangles, numTriangles * sizeof(Triangle));

    // copy scene parts to vram
    cudaMemcpy(h_scene.spheres, spheres.data(), h_scene.sphereNum * sizeof(RTSphere), cudaMemcpyHostToDevice);
    cudaMemcpy(h_scene.planes, planes.data(), h_scene.planeNum * sizeof(RTPlane), cudaMemcpyHostToDevice);
    // cudaMemcpy(h_scene.vertices, h_vertices, numVertices * sizeof(float3), cudaMemcpyHostToDevice);
    // cudaMemcpy(h_scene.triangles, h_triangles, numTriangles * sizeof(Triangle), cudaMemcpyHostToDevice);

    // allocate Scene struct on GPU and copy it
    cudaMalloc(&d_scene, sizeof(RTScene));
    cudaMemcpy(d_scene, &h_scene, sizeof(RTScene), cudaMemcpyHostToDevice);

    // RTScene *dataGpu;
    // cudaMalloc(&dataGpu, transformedData.size());
    // cudaMemcpy(dataGpu, &transformedData, transformedData.size(), cudaMemcpyHostToDevice);

    debugString += t.format("Device malloc and copy: # ($)\n");

    calculatePixel<<<grid, block>>>(d_scene, d_output);
    cudaDeviceSynchronize();
    debugString += t.format("Rendering: # ($)\n");

    cudaMemcpy(output, d_output, rasterSize, cudaMemcpyDeviceToHost);

    cudaFree(h_scene.spheres);
    cudaFree(d_scene);
    cudaFree(d_output);

    debugString += t.format("Copying the result and freeing device memory: # ($)\n");
}

} // namespace CudaRT
