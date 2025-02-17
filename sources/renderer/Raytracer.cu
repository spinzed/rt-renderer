#include <cuda_runtime.h>
#include <iostream>

__global__ void addKernel(int* a, int* b, int* c) {
    int idx = threadIdx.x;
    c[idx] = a[idx] + b[idx];
}

extern "C" void launchAddKernel(int* a, int* b, int* c, int size) {
    int *d_a, *d_b, *d_c;
    cudaMalloc(&d_a, size * sizeof(int));
    cudaMalloc(&d_b, size * sizeof(int));
    cudaMalloc(&d_c, size * sizeof(int));

    cudaMemcpy(d_a, a, size * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, size * sizeof(int), cudaMemcpyHostToDevice);

    addKernel<<<1, size>>>(d_a, d_b, d_c);

    cudaMemcpy(c, d_c, size * sizeof(int), cudaMemcpyDeviceToHost);

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);
}
