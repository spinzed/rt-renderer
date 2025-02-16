#include "core/Transform.h"
#include "utils/mtr.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

glm::vec3 Transform::right() { return matrix[0]; }

glm::vec3 Transform::up() { return matrix[1]; }

glm::vec3 Transform::forward() { return -matrix[2]; }

glm::vec3 Transform::position() { return matrix[3]; }

glm::vec3 Transform::vertical() { return glm::transpose(matrix)[1]; } // something's fishy here

glm::vec3 Transform::direction() { return glm::cross(TransformIdentity::up(), right()); }

void Transform::setRight(glm::vec3 right) {
    matrix[0] = glm::vec4(right, 0);
    setMatrix(matrix);
}
void Transform::setUp(glm::vec3 up) {
    matrix[1] = glm::vec4(up, 0);
    setMatrix(matrix);
}
void Transform::setForward(glm::vec3 forward) {
    matrix[2] = glm::vec4(-forward, 0);
    setMatrix(matrix);
}

void Transform::translate(glm::vec3 position) { setMatrix(calculateTranslated(position)); }

glm::mat4 Transform::calculateTranslated(glm::vec3 position) { return glm::translate(matrix, position); }

void Transform::setPosition(glm::vec3 position) {
    matrix[3][0] = position[0];
    matrix[3][1] = position[1];
    matrix[3][2] = position[2];
    setMatrix(matrix);
}

void Transform::rotate(glm::vec3 axis, float degrees) { setMatrix(calculateRotated(axis, degrees)); }

glm::mat4 Transform::calculateRotated(glm::vec3 axis, float degrees) {
    return glm::rotate(matrix, glm::radians(degrees), axis);
}

glm::vec3 Transform::getScale() {
    glm::vec3 scale;

    scale.x = glm::length(glm::vec3(right()));
    scale.y = glm::length(glm::vec3(up()));
    scale.z = glm::length(glm::vec3(forward()));

    return scale;
}

void Transform::scale(glm::vec3 scale) { setMatrix(calculateScaled(scale)); }

glm::mat4 Transform::calculateScaled(glm::vec3 scale) { return glm::scale(matrix, scale); }

void Transform::scale(float sc) { setMatrix(calculateScaled(sc)); }

glm::mat4 Transform::calculateScaled(float sc) { return calculateScaled(glm::vec3(sc)); }

void Transform::setScale(glm::vec3 sc) {
    glm::vec3 scurrent = getScale();
    if (scurrent == glm::vec3(0))
        return;
    scale(sc / scurrent);
}

void Transform::setScale(float sc) { setScale(glm::vec3(sc)); }

void Transform::pointAt(glm::vec3 point) { return pointAt(point, up()); }

void Transform::pointAt(glm::vec3 point, glm::vec3 up) { return pointAtDirection(point - position(), up); }

void Transform::pointAtDirection(glm::vec3 direction) { return pointAtDirection(direction, up()); }

void Transform::pointAtDirection(glm::vec3 direction, glm::vec3 up) {
    glm::vec3 forward = glm::normalize(-direction);
    glm::vec3 upNorm = glm::normalize(up);

    glm::vec3 right = glm::normalize(glm::cross(upNorm, forward));
    upNorm = glm::cross(forward, right);

    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix[0] = glm::vec4(right, 0.0f);
    rotationMatrix[1] = glm::vec4(upNorm, 0.0f);
    rotationMatrix[2] = glm::vec4(forward, 0.0f);

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position());
    transform *= rotationMatrix;
    transform = glm::scale(transform, getScale());

    setMatrix(transform);
}

void Transform::normalize(glm::vec3 min, glm::vec3 max) {
    float x = (max.x + min.x) / 2;
    float y = (max.y + min.y) / 2;
    float z = (max.z + min.z) / 2;

    float M = std::max(max.x - min.x, max.y - min.y);
    M = std::max(M, max.z - min.z);

    matrix = calculateTranslated(glm::vec3(-x, -y, -z));
    matrix = calculateScaled(2 / M);

    setMatrix(matrix);
}

glm::mat4 Transform::frustum(float left, float right, float bottom, float top, float nearp, float far) {
    return mtr::frustum(left, right, bottom, top, nearp, far);
}

glm::mat4 Transform::perspective(int width, int height, float nearp, float far, float angleDeg) {
    // matrix = glm::perspective(glm::radians(70.0f), (float)width/(float)height, 0.1f, 100.0f);
    float w = nearp * tan(glm::radians(angleDeg / 2));
    float h = (float)height / width * w;
    return Transform::frustum(-w, w, -h, h, nearp, far);
}

glm::mat4 Transform::getMatrix() { return matrix; }

glm::vec3 Transform::getEulerAngles() {
    glm::vec3 angles(0);
    float dotProduct = glm::dot(forward(), direction());
    dotProduct = glm::clamp(dotProduct, -1.0f, 1.0f);
    float isvert = glm::dot(forward(), TransformIdentity::up());
    angles.x = std::acos(dotProduct) * glm::sign(isvert);

    dotProduct = glm::dot(forward(), TransformIdentity::forward());
    dotProduct = glm::clamp(dotProduct, -1.0f, 1.0f);
    angles.y = std::acos(dotProduct);

    return angles * (180.0f / glm::pi<float>());
}
