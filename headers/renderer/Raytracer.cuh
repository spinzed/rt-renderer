#pragma once

#include <renderer/RenderTypes.h>
#include <vector_types.h>

struct RTRay {
    float3 origin;
    float3 direction;
};

struct RTObject {
    int type;
    int offset;
};

struct RTMesh {};

struct RTSphere {
    float3 center;
    float radius;
    float3 color;
};

struct RTPlane {
    float3 center;
    float3 normal;
    float width;
    float height;
    float3 u;
    float3 v;
    float3 color;
};

struct RTCamera {
    RTRay ray;
    float3 topLeft;
    float3 dx;
    float3 dy;
};

struct RTScene {
    RTCamera camera;
    int sphereNum;
    RTSphere *spheres;
    int planeNum;
    RTPlane *planes;
    // int objectNum;
    // RTObject *objects;
    // int dataSize;
    // void *data;
    // size_t size() { return objectNum * sizeof(RTObject) + dataSize; }
};

namespace CudaRT {
inline std::string debugString;
void render(int width, int height, RenderData data, float *output);
} // namespace CudaRT
