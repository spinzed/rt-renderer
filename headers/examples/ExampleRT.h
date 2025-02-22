#pragma once

#include <string>

#include "core/Engine.h"
#include "core/Input.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"
#include "utils/PerlinNoise.h"
#include "utils/Timer.h"
#include "utils/mtr.h"

class ExampleRT {
  public:
    int width = 1000, height = 1000;
    float moveSensitivity = 10, sprintMultiplier = 5, mouseSensitivity = 0.15f;

    Engine *engine;

    int run(std::string dir);
    void cursorPositionCallback(WindowCursorEvent event);
};
