#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

#include <iostream>

Camera::Camera(int width, int height) {
    setSize(width, height);
    recalculateMatrix();
}

void Camera::setSize(int width, int height) {
    constraints.nearp = 0.1f;
    constraints.far = 100.0f;
    constraints.angle = 70.0f;
    float w = constraints.nearp * tan(glm::radians(constraints.angle / 2));
    float h = (float)height / width * w;
    constraints.left = -w;
    constraints.right = w;
    constraints.bottom = -h;
    constraints.top = h;
    projectionMatrix = Transform::frustum(constraints.left, constraints.right, constraints.bottom, constraints.top,
                                          constraints.nearp, constraints.far);
    recalculateMatrix();
}

void Camera::rotate(float degreesX, float degreesY) {
    Transform::rotate(vertical(), degreesX);
    Transform::rotate(TransformIdentity::right(), degreesY);
    glm::vec3 a = getEulerAngles();
    float pitch = a[0];
    if (pitch < -90) { // down
        setForward(-TransformIdentity::up());
        setUp(glm::cross(TransformIdentity::up(), right()));
    }
    if (pitch > 90) { // up
        setForward(TransformIdentity::up());
        setUp(glm::cross(right(), TransformIdentity::up()));
    }
}

glm::mat4 Camera::getViewMatrix() { return glm::inverse(getMatrix()); }

glm::mat4 Camera::getProjectionMatrix() { return projectionMatrix; }

glm::mat4 Camera::getProjectionViewMatrix() { return projectionViewMatrix; }

void Camera::recalculateMatrix() {
    projectionViewMatrix = projectionMatrix * getViewMatrix();
    notify();
}
