#pragma once

#include "models/Raster.h"
#include "objects/FullscreenTexture.h"
#include "renderer/RenderTypes.h"

#include "utils/ThreadPool.h"
#include "utils/Timer.h"

#define RAYTRACE_MULTICORE 1
#define RAYTRACE_DEPTH 2
#define RAYTRACE_NUM_OF_SAMPLES 1
#define RAYTRACE_RANDOMNESS 0
#define RAYTRACE_OFFSET 1
#define RAYTRACE_AMBIENT glm::vec3(0.2, 0.2, 0.2)

#define K_ROUGNESS 0.0f // pathrace diffusion
#define K_SPECULAR 0.0f // reflection
#define k_transmit 0.0f // translucency

class Raytracer {
  public:
    void Init(int width, int height);
    void Render(RenderData data, FullscreenTexture *output);
    void SetResolution(int width, int height);

    unsigned int getDepth() { return depth; };
    void setDepth(int d) { depth = d; };

    bool integrationEnabled() { return monteCarlo; }
    void setIntegrationEnabled(bool enabled) {
        monteCarlo = enabled;
        if (enabled) {
            resetStats();
            InactiveRaster()->reset();
        }
    }

    struct HardwareConstants {
      int gridSize = 16;
    } hardware;

    RTRenderSettings settings = {
      .renderSpheres = true,
      .renderPlanes = true,
      .renderMeshes = true,
    };

    float kSpecular() { return k_specular; }
    void setKSpecular(float k) { k_specular = k; }

    float kRoughness() { return k_roughness; }
    void setKRougness(float k) { k_roughness = k; }

    void resetStats();

    void line(RenderData data, glm::vec3 current, glm::vec3 dx, glm::vec3 dy, int i);

    glm::vec3 phong(Intersection &p, glm::vec3 diffuseColor, RenderData data);
    std::optional<Intersection> raycast(RenderData data, glm::vec3 origin, glm::vec3 direction,
                                        Object *&intersectedObject);

    glm::vec3 raycast(RenderData data, glm::vec3 origin, glm::vec3 direction);              // returns color
    glm::vec3 raytrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth);  // returns color
    glm::vec3 pathtrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth); // returns color

    int width = 0;
    int height = 0;

    std::string debugString;
    int depth = RAYTRACE_DEPTH;
    int rpp = 1;

  private:
    bool monteCarlo = false;
    float k_specular = K_SPECULAR;
    float k_roughness = K_ROUGNESS;

    Timer t = Timer::start();
    ThreadPool *pool = nullptr;

    inline static unsigned int renderCount = 0;
    inline static double totalTime = 0;

    inline static std::vector<Raster<float> *> rasteri;
    inline static int currentRasterIndex = 0;

    glm::vec3 clearColor;

    void SoftwareRender(RenderData data);
    void HardwareRender(RenderData data);

    void MonteCarlo();

    Raster<float> *CurrentRaster();
    Raster<float> *InactiveRaster();
    void SwitchRaster();
};
