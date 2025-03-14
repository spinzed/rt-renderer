#pragma once

#include <renderer/RenderTypes.h>

#include <cuda_runtime.h>
#include <vector_types.h>

struct RTRay {
    float3 origin;
    float3 direction;
};

struct RTObject {
    int type;
    int offset;
};

struct RTMesh {
    int indexOffset;
    int indexNumber;
    int uvOffset;
    int materialIndex;
};

struct RTSphere {
    float3 center;
    float radius;
    int materialIndex;
};

struct RTPlane {
    float3 center;
    float3 normal;
    float width;
    float height;
    float3 u;
    float3 v;
    int materialIndex;
};

struct RTCamera {
    RTRay ray;
    float3 topLeft;
    float3 dx;
    float3 dy;
};

struct RTTexture {
    int width;
    int height;
    int channels; // 3 or 4
    float *data;  // the texture exists if data isn't nullptr

    __device__ __host__ float3 getPixel(int x, int y) {
        int pixNum = channels * (y * width + x);
        if (pixNum < 0 || pixNum > width * height * channels) {
            return make_float3(1, 1, 0);
        }
        return make_float3(data[pixNum], data[pixNum + 1], data[pixNum + 2]);
    }

    __device__ __host__ float3 getElement(float u, float v) {
        if (u < 0 || u >= 1)
            return make_float3(1, 0, 0);
        if (v < 0 || v >= 1)
            return make_float3(0, 1, 0);
        // return make_float3(data[0], data[1], data[2]);
        return getPixel((int)(u * width), (int)(v * height));
    }
};

struct RTMaterial {
    float reflectivity;
    // float shininess;
    float3 emission;
    float emissionStrength;
    float3 diffuseColor;
    int diffuseTextureIndex; // -1 if not present, use diffuseColor instead
    float smoothness;
};

struct RTScene {
    RTCamera camera;
    int sphereNum;
    RTSphere *spheres;
    int planeNum;
    RTPlane *planes;
    int meshNum;
    RTMesh *meshes;
    float *vertices;
    float *uvs;
    int *indices;
    RTMaterial *materials;
    RTTexture *textures;
};

namespace CudaRT {

inline std::string debugString;
inline int gridSize = 256;
inline RTRenderSettings settings;

void init();
void render(int width, int height, int depth, int rpp, RenderData data, float *output);
void monteCarlo(int width, int height, int rednerCount, float *input1, float *input2); // output written to input1
} // namespace CudaRT
