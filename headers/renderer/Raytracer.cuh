#pragma once

#include <renderer/RenderTypes.h>

struct RTObject {
    int type;
    int offset;
};

// Mesh - type 0
struct RTMesh {};

// Sphere - type 1
struct RTSphere {
    float center[3];
    float radius;
};

// Plane - type 2
struct RTPlane {};

namespace CudaRT {
void render(int width, int height, RenderData data, float *output);
}
