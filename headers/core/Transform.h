#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
  public:
    constexpr Transform() = default;
    Transform(glm::mat4 m) { matrix = m; };
    Transform(Transform *t) { matrix = t->matrix; };
    Transform(Transform &t) { matrix = t.matrix; };

    // glm::mat4 matrix = glm::mat4(1);
    glm::mat4 matrix = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

    glm::vec3 right();
    glm::vec3 up();
    glm::vec3 forward();
    glm::vec3 position();

    glm::vec3 vertical();
    glm::vec3 direction();
    glm::vec3 getScale();

    void setRight(glm::vec3 right);
    void setUp(glm::vec3 up);
    void setForward(glm::vec3 forward);

    void translate(glm::vec3 orientation);
    void setPosition(glm::vec3 position);
    void rotate(glm::vec3 axis, float degrees);
    void scale(glm::vec3 scale);
    void scale(float scale);
    void setScale(glm::vec3 scale);
    void setScale(float scale);
    void pointAt(glm::vec3 point);
    void pointAt(glm::vec3 point, glm::vec3 up);
    void pointAtDirection(glm::vec3 direction);
    void pointAtDirection(glm::vec3 direction, glm::vec3 up);
    void normalize(glm::vec3 min, glm::vec3 max);

    // doesn't modify the object
    glm::mat4 calculateRotated(glm::vec3 axis, float degrees);
    glm::mat4 calculateScaled(glm::vec3 scale);
    glm::mat4 calculateScaled(float);
    glm::mat4 calculateTranslated(glm::vec3 scale);

    static glm::mat4 frustum(float l, float r, float b, float t, float n, float f);
    static glm::mat4 perspective(int width, int height, float nearp, float far, float angleDeg);

    glm::highp_mat4 getMatrix();
    void setMatrix(glm::mat4 m) {
        matrix = m;
        for (const auto &f : listeners) {
            f();
        }
    };
    Transform copy() { return Transform(matrix); }
    void copyFrom(Transform *t) { matrix = t->matrix; }
    void copyFrom(Transform &t) { matrix = t.matrix; }

    glm::vec3 getEulerAngles(); // pitch, jaw, roll

    void addListener(std::function<void(void)> f) { listeners.push_back(f); }

  private:
    std::vector<std::function<void(void)>> listeners;
};

class TransformIdentity : public Transform {
  private:
    static constexpr Transform singleton() { return Transform(); };

    TransformIdentity() {};

  public:
    static glm::vec3 right() { return glm::transpose(singleton().matrix)[0]; }
    static glm::vec3 up() { return glm::transpose(singleton().matrix)[1]; }
    static glm::vec3 forward() { return -glm::transpose(singleton().matrix)[2]; }
    static glm::vec3 position() { return singleton().matrix[3]; }
    static glm::vec3 vertical() { return singleton().vertical(); }
    static glm::mat4 getMatrix() { return singleton().getMatrix(); }
};
