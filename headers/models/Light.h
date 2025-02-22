#pragma once

#include "core/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer/Cubemap.h"

#include <vector>

class Light {
  public:
    Transform t;
    glm::vec3 intensity;
    glm::vec3 color;
    float range;

    Transform *getTransform() { return &t; }

    virtual ~Light() = default;

  protected:
    Light(glm::vec3 position, glm::vec3 intensity, glm::vec3 color, float range) {
        t.setPosition(position);
        this->intensity = intensity;
        this->color = color;
        this->range = range;
    }
};

class PointLight : public Light {
  public:
    Cubemap cb;
    std::vector<glm::mat4> transforms;

    float aspect = 1.0f;
    float nearPlane = 0.2f;
    float farPlane;

    PointLight(glm::vec3 position, glm::vec3 intensity, glm::vec3 color, float range = 10.0f)
        : Light(position, intensity, color , range), cb(1024, 1024, 3, true) {
        farPlane = range;

        t.addListener(std::bind(&PointLight::calculateMatrices, this));
        calculateMatrices();
    }

  private:
    void calculateMatrices() {
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);
        glm::vec3 position = t.position();

        glm::mat4 lookAtPosX =
            projection * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        glm::mat4 lookAtNegX =
            projection * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        glm::mat4 lookAtPosY =
            projection * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 lookAtNegY =
            projection * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        glm::mat4 lookAtPosZ =
            projection * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        glm::mat4 lookAtNegZ =
            projection * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        transforms = {lookAtPosX, lookAtNegX, lookAtPosY, lookAtNegY, lookAtPosZ, lookAtNegZ};
    }
};
