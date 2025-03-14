#pragma once

#include "objects/MeshObject.h"
#include "utils/Types.h"

#include <optional>

class Sphere : public MeshObject {
  public:
    Sphere(std::string name, glm::vec3 center, float radius, glm::vec3 color);
    Sphere(std::string name, glm::vec3 center, float radius);
    ~Sphere();

    Mesh *getMesh() { return mesh; }

    virtual std::optional<Intersection> findIntersection(glm::vec3 origin, glm::vec3 direction);

    glm::vec3& color();

  private:
    void init(std::string name, glm::vec3 center, float radius, glm::vec3 color);
    void generateSphere();
};
