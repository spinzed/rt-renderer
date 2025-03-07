#pragma once

#include "models/Light.h"
#include "objects/FullscreenTexture.h"
#include "objects/Object.h"
#include "objects/Skybox.h"
#include "renderer/Framebuffer.h"
#include "renderer/Raytracer.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

enum RenderingMethod {
    Noop,
    Rasterize,
    Raycast,
    Raytrace,
    Pathtrace,
};

typedef glm::vec3 (*RayStrategy)(glm::vec3, glm::vec3, int);

// Nenjin
class Renderer {
  public:
    static void Init(int width, int height);

    static void Render(RenderData data);

    inline static int vsync = 0;
    static void SetResolution(int width, int height);

    inline static enum RenderingMethod RenderingMethod() { return method; };
    inline static void SetRenderingMethod(enum RenderingMethod m) { method = m; };

    static void rasterize(RenderData data);
    inline static bool _cameraMatrixChanged = true;

    inline static Raytracer raytracer;

    inline static std::string debugString;

  private:
    inline static int _width;
    inline static int _height;
    inline static glm::vec3 _clearColor;

    inline static Shader *lightMapShader;

    inline static enum RenderingMethod method = RenderingMethod::Rasterize;

    inline static Texture *outputTexture = nullptr;
    inline static FullscreenTexture *textureShower = nullptr;
    inline static Framebuffer *depthFramebuffer = nullptr;
    inline static Shader *rt = nullptr;
    inline static PointLight *light = nullptr;

    inline static std::vector<float> lightPositions;
    inline static std::vector<float> lightIntensities;
    inline static std::vector<float> lightColors;

    static void UpdateShader(Object *object, RenderData data);
};
