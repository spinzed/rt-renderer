#pragma once

#include "core/Transform.h"

#include <functional>
#include <vector>

typedef struct {
    float nearPlane;
    float farPlane;
    float angle;
    float left;
    float right;
    float bottom;
    float top;
} CameraConstraints;

class Camera : public Transform {
  private:
    glm::mat4 projectionMatrix;
    glm::mat4 projectionViewMatrix;

    std::vector<std::function<void()>> observers;

    void notify() {
        for (auto o : observers) {
            o();
        }
    };

  public:
    Camera(int width, int height);
    //~Camera();

    CameraConstraints constraints;

    void setSize(int width, int height);

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    glm::mat4 getProjectionViewMatrix();
    void recalculateMatrix();

    void addChangeListener(std::function<void()> observer) { observers.push_back(observer); };

    using Transform::rotate;
    void rotate(float degreesX, float degreesY);
};