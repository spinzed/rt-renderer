#pragma once

#include "objects/Skybox.h"
#include "models/Light.h"
#include "objects/Object.h"
#include "core/Camera.h"

#include <vector>

struct RenderData {
    std::vector<Object *> *objects;
    std::vector<Light *> *lights;
    Skybox *skybox;
    Camera *camera;
};
