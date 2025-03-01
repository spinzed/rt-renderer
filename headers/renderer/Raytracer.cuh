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

// Mesh - type 0
struct RTMesh {};

// Sphere - type 1
struct RTSphere {
    float3 center;
    float radius;
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
    //int objectNum;
    //RTObject *objects;
    //int dataSize;
    //void *data;
    //size_t size() { return objectNum * sizeof(RTObject) + dataSize; }
};

// Plane - type 2
struct RTPlane {};

namespace CudaRT {
void render(int width, int height, RenderData data, float *output);
}
