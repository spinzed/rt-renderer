#pragma once

#include "core/Camera.h"
#include "core/ParticleSystem.h"
#include "core/WindowManager.h"
#include "models/Light.h"
#include "objects/Object.h"
#include "objects/Skybox.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

class Engine {
  public:
    static void Init(int width, int height, std::string execDirectory);

    static void SetResolution(int width, int height);
    static void SetSkybox(Cubemap *cb) { skybox = new Skybox(cb); };

    static void AddObject(Object *o);
    static void RemoveObject(Object *o);
    static void AddLight(Light *l);
    static void AddParticleCluster(ParticleCluster *pc);

    static Camera *GetCamera();
    static void onCameraChange(); // private?

    static void Loop();
    static void Render();
    static void Clear();
    static void SetShouldClose(); // marks the engine close after the current frame finishes rendering

    inline static int vsync = 0;
    static void EnableVSync();
    static void DisableVSync();
    static void SwapBuffers();

    inline static bool guiEnabled = false;
    static void SetGUIEnabled(bool);

    inline static WindowManager *manager = nullptr;

  private:
    inline static std::vector<Object *> objects;
    inline static std::vector<Light *> lights;
    inline static Skybox *skybox = nullptr;
    inline static std::shared_ptr<Camera> camera;

    inline static int cursorWasHidden;
};
