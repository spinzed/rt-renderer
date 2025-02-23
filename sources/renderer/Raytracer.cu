#include "renderer/Raytracer.cuh"

#include <cuda_runtime.h>
#include <iostream>

__constant__ int cwidth;
__constant__ int cheight;

__global__ void calculatePixel(float *c) {
    int x = blockIdx.x;
    int y = threadIdx.x;

    if (x < cwidth && y < cheight) {
        c[3 * (y * cwidth + x)] = (float)x / 1024;
        c[3 * (y * cwidth + x) + 1] = (float)y / 1024;
        c[3 * (y * cwidth + x) + 2] = 0;
    }
}

namespace CudaRT {

void render(int width, int height, RenderData data, float *output) {
    float *rendered;
    int rasterSize = width * height * 3 * sizeof(float);
    cudaMalloc(&rendered, rasterSize);

    cudaMemcpyToSymbol(cwidth, &width, sizeof(int), 0, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(cheight, &height, sizeof(int), 0, cudaMemcpyHostToDevice);

    // render logic
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

    calculatePixel<<<grid, block>>>(rendered);
    cudaDeviceSynchronize();
    std::cout << "hewwo" << std::endl;

    cudaMemcpy(output, rendered, rasterSize, cudaMemcpyDeviceToHost);

    cudaFree(rendered);
}

} // namespace CudaRT
