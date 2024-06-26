#pragma once

#include "Mesh.h"
#include "Shader.h"
#include "Transform.h"

class Light {
  public:
    float position[3];
    float intensity[3];
    float color[3];

    Light(float x, float y, float z, float i1, float i2, float i3, float r, float g, float b) {
        position[0] = x;
        position[1] = y;
        position[2] = z;
        intensity[0] = i1;
        intensity[1] = i2;
        intensity[2] = i3;
        color[0] = r;
        color[1] = g;
        color[2] = b;
    }
};

class Object {
  private:
    Transform *transform;
    bool setViewMatrix = false;
    int primitiveType = GL_TRIANGLES;

  public:
    Shader *shader;
    Mesh *mesh;

    Object();
    Object(Mesh *m, Shader *s);
    ~Object();

    IntersectPoint intersectPoint(glm::vec3 origin, glm::vec3 direction) {
        return mesh->intersectPoint(origin, direction, getModelMatrix());
    }

    virtual void render();
    virtual void render(Shader *s);

    glm::mat4 getModelMatrix() { return transform->getMatrix(); };
    Transform *getTransform() { return transform; };
    Mesh *getMesh() { return mesh; };

    void setPrimitiveType(int type) { primitiveType = type; }
};