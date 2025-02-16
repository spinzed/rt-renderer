#pragma once

#include "core/Camera.h"
#include "models/Light.h"
#include "objects/FullscreenTexture.h"
#include "objects/Object.h"
#include "renderer/Framebuffer.h"
#include "utils/ThreadPool.h"
#include "utils/Timer.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <objects/Skybox.h>
#include <optional>

#define RAYTRACE_MULTICORE 1
#define RAYTRACE_DEPTH 1
#define RAYTRACE_NUM_OF_SAMPLES 1
#define RAYTRACE_RANDOMNESS 0
#define RAYTRACE_OFFSET 1
#define RAYTRACE_AMBIENT glm::vec3(0.2, 0.2, 0.2)

#define K_ROUGNESS 0.0f // pathrace diffusion
#define K_SPECULAR 0.0f // reflection
#define k_transmit 0.0f // translucency

enum RenderingMethod {
    Noop,
    Rasterize,
    Raycast,
    Raytrace,
    Pathtrace,
};

typedef glm::vec3 (*RayStrategy)(glm::vec3, glm::vec3, int);

struct RenderData {
    std::vector<Object *> *objects;
    std::vector<Light *> *lights;
    Skybox *skybox;
    Camera *camera;
};

// Nenjin
class Renderer {
  public:
    static void Init(int width, int height);

    static void Render(RenderData data);
    static void rasterize(RenderData data);
    static void rayRender(RenderData data);

    inline static int vsync = 0;
    static void EnableVSync();
    static void DisableVSync();
    static void SwapBuffers();
    static void SetResolution(int width, int height);

    inline static void resetStats() {
        renderCount = 0;
        totalTime = 0;
    }

    inline static unsigned int getDepth() { return depth; };
    inline static void setDepth(int d) { depth = d; };

    inline static unsigned int getRenderingMethod() { return method; };
    inline static void setRenderingMethod(RenderingMethod m) { method = m; };

    inline static unsigned int integrationEnabled() { return monteCarlo; }
    inline static void setIntegrationEnabled(bool enabled) { monteCarlo = enabled; }

    inline static float kSpecular() { return k_specular; }
    inline static void setKSpecular(float k) { k_specular = k; }

    inline static float kRoughness() { return k_roughness; }
    inline static void setKRougness(float k) { k_roughness = k; }

    static void line(RenderData data, glm::vec3 current, glm::vec3 dx, glm::vec3 dy, int i);

    static glm::vec3 phong(Intersection &p, glm::vec3 diffuseColor, RenderData data);
    static std::optional<Intersection> raycast(RenderData data, glm::vec3 origin, glm::vec3 direction, Object *&intersectedObject);

    static glm::vec3 raycast(RenderData data, glm::vec3 origin, glm::vec3 direction);              // returns color
    static glm::vec3 raytrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth);  // returns color
    static glm::vec3 pathtrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth); // returns color
    static void iscrtajRaster();
    static void spremiRaster();

    inline static bool _cameraMatrixChanged = true;

  private:
    inline static int _width;
    inline static int _height;
    inline static glm::vec3 _clearColor;

    inline static Shader *lightMapShader;

    inline static unsigned int depth = 0;
    inline static RenderingMethod method;

    inline static unsigned int renderCount = 0;
    inline static bool monteCarlo = false;
    inline static float totalTime = 0;

    inline static float k_specular = K_SPECULAR;
    inline static float k_roughness = K_ROUGNESS;

    inline static std::vector<Raster<float> *> rasteri;
    inline static int currentRasterIndex = 0;

    inline static Texture *outputTexture = nullptr;
    inline static FullscreenTexture *textureShower = nullptr;
    inline static Framebuffer *depthFramebuffer = nullptr;
    inline static Shader *rt = nullptr;
    inline static PointLight *light = nullptr;

    inline static Timer t = Timer::start();
    inline static ThreadPool *pool = nullptr;

    inline static std::vector<float> lightPositions;
    inline static std::vector<float> lightIntensities;
    inline static std::vector<float> lightColors;

    static void UpdateShader(Object *object, RenderData data);
};
