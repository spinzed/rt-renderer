#include "renderer/Raytracer.cuh"

#include <cuda_runtime.h>
#include <iostream>

// additional utility functions
inline float3 vec3_to_float3(glm::vec3 v) { return make_float3(v.x, v.y, v.z); }

// for some reason dot is not defined
__device__ __host__ inline float dot(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// additional operators for float3
__device__ float3 operator+(const float3 &a, const float3 &b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }

__device__ float3 operator-(const float3 &a, const float3 &b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }

__device__ float3 operator*(const float3 &a, const int &b) { return make_float3(a.x * b, a.y * b, a.z * b); }

__device__ float3 operator*(const int &b, const float3 &a) { return make_float3(a.x * b, a.y * b, a.z * b); }

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

__device__ int findClosestIntersection(const RTRay &ray, RTSphere *spheres, int numSpheres, float &closestT) {
    int closestSphereIdx = -1;
    closestT = 1e30f; // Large initial value

    for (int i = 0; i < numSpheres; i++) {
        float t;
        if (intersectSphere(ray, spheres[i], t) && t < closestT) {
            closestT = t;
            closestSphereIdx = i;
        }
    }

    return closestSphereIdx;
}

__constant__ int cwidth;
__constant__ int cheight;

__global__ void calculatePixel(RTScene *scene, float *output) {
    int x = blockIdx.x;
    int y = threadIdx.x;

    if (!(x < cwidth && y < cheight))
        return;

    float3 pos = scene->camera.topLeft + x * scene->camera.dx + (cheight-y) * scene->camera.dy;
    RTRay ray = {
        .origin = scene->camera.ray.origin,
        .direction = pos - scene->camera.ray.origin,
    };

    bool intersect = false;
    RTSphere sp = {1, 1, 1, 2};

    for (int i = 0; i < scene->sphereNum; i++) {
        float t;
        if (intersectSphere(ray, scene->spheres[i], t)) {
            intersect = true;
        }
    }
    // float t;
    // if (intersectSphere(scene->camera, sp, t)) {
    //     intersect = true;
    // }

    // output[3 * (y * cwidth + x)] = (float)x / gridDim.x;
    // output[3 * (y * cwidth + x) + 1] = (float)y / blockDim.x;
    // output[3 * (y * cwidth + x) + 2] = 0;
    float3 color = intersect ? make_float3(0, 1, 0) : make_float3(1, 0, 0);

    int firstComp = 3 * (y * cwidth + x);
    output[firstComp] = color.x;
    output[firstComp + 1] = color.y;
    output[firstComp + 2] = color.z;
}

// exposed functions

namespace CudaRT {

void render(int width, int height, RenderData data, float *output) {
    float *d_output;
    int rasterSize = width * height * 3 * sizeof(float);
    cudaMalloc(&d_output, rasterSize);

    cudaMemcpyToSymbol(cwidth, &width, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(cheight, &height, sizeof(int), 0, cudaMemcpyHostToDevice);

    // precompute screen positions in world space
    Camera *camera = data.camera;
    glm::vec3 camPos = camera->position();
    CameraConstraints c = camera->constraints;
    glm::vec3 start = camPos + c.nearPlane * camera->forward() + c.top * camera->up() + c.left * camera->right();
    glm::vec3 current = start;
    glm::vec3 row = (c.right - c.left) * camera->right();
    glm::vec3 dx = row * (1.0f / width);
    glm::vec3 column = -(c.top - c.bottom) * camera->up();
    glm::vec3 dy = column * (1.0f / height);

    int grid = 1024;
    int block = 1024;

    // for (Object* obj: *data.objects) {
    // if (obj->type == "sphere") {
    //     int position = obj->getFullTransform().position();
    //     rawData.emplace_back();
    // }
    //}

    std::vector<RTSphere> spheres(1);
    spheres[0] = {.center = make_float3(1, 1, 1), .radius = 2};

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
        .spheres = nullptr, // point to d_spheres
    };
    RTScene *d_scene;

    // Allocate GPU memory for scene
    cudaMalloc(&h_scene.spheres, h_scene.sphereNum * sizeof(RTSphere));
    // cudaMalloc(&h_scene.planes, numPlanes * sizeof(Plane));
    // cudaMalloc(&h_scene.vertices, numVertices * sizeof(float3));
    // cudaMalloc(&h_scene.triangles, numTriangles * sizeof(Triangle));

    // Copy object data
    cudaMemcpy(h_scene.spheres, spheres.data(), h_scene.sphereNum * sizeof(RTSphere), cudaMemcpyHostToDevice);
    // cudaMemcpy(h_scene.planes, h_planes, numPlanes * sizeof(Plane), cudaMemcpyHostToDevice);
    // cudaMemcpy(h_scene.vertices, h_vertices, numVertices * sizeof(float3), cudaMemcpyHostToDevice);
    // cudaMemcpy(h_scene.triangles, h_triangles, numTriangles * sizeof(Triangle), cudaMemcpyHostToDevice);

    // Allocate Scene struct on GPU and copy it
    cudaMalloc(&d_scene, sizeof(RTScene));
    cudaMemcpy(d_scene, &h_scene, sizeof(RTScene), cudaMemcpyHostToDevice);

    // RTScene *dataGpu;
    // cudaMalloc(&dataGpu, transformedData.size());
    // cudaMemcpy(dataGpu, &transformedData, transformedData.size(), cudaMemcpyHostToDevice);

    calculatePixel<<<grid, block>>>(d_scene, d_output);
    cudaDeviceSynchronize();

    cudaMemcpy(output, d_output, rasterSize, cudaMemcpyDeviceToHost);

    cudaFree(h_scene.spheres);
    cudaFree(d_scene);
    cudaFree(d_output);
}

} // namespace CudaRT
