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

struct RTMesh {
    int indexOffset;
    int indexNumber;
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

struct RTMaterial {
    float reflectivity;
    //float shininess;
    float3 emission;
    float emissionStrength;
    float3 diffuse;
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
    int *indices;
    RTMaterial *materials;
};

namespace CudaRT {

inline std::string debugString;
inline int gridSize = 256;
inline RTRenderSettings settings;

void init();
void render(int width, int height, int depth, int rpp, RenderData data, float *output);
void monteCarlo(int width, int height, int rednerCount, float *input1, float *input2); // output written to input1
} // namespace CudaRT
