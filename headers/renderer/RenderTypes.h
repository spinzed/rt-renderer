#pragma once

#include "models/Light.h"
#include "renderer/Camera.h"
#include "renderer/Texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

struct Material {
    glm::vec3 colorAmbient;
    glm::vec3 colorDiffuse;
    glm::vec3 colorSpecular;
    float shininess;
    float smoothness;
    glm::vec3 colorReflective;
    glm::vec3 colorEmissive;
    float emissiveStrength;
    glm::vec3 colorTransmitive;
    Texture *texture = nullptr;
};

class Object;
class Light;
class Skybox;

struct RenderData {
    std::vector<Object *> *objects;
    std::vector<Light *> *lights;
    Skybox *skybox;
    Camera *camera;
};

struct RTRenderSettings {
    bool renderSpheres;
    bool renderPlanes;
    bool renderMeshes;
    float blurriness;
    float dofStrength;
    float dofDistance;
};
