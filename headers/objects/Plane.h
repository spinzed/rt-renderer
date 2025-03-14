#pragma once

#include "glm/geometric.hpp"
#include "glm/glm.hpp"
#include "objects/MeshObject.h"

class Plane : public MeshObject {
  public:
    Plane(std::string name, float width, float height, glm::vec3 color)
        : MeshObject(name, new Mesh(GL_TRIANGLES), Shader::Load("phong")) {
        init(name, width, height, color);
    }
    Plane(std::string name, float width, float height)
        : MeshObject(name, new Mesh(GL_TRIANGLES), Shader::Load("phong")) {
        init(name, width, height, glm::vec3(1));
    }
    ~Plane() {};

    inline glm::vec3 &color() { return material->colorDiffuse; }

    glm::vec3 normal() { return glm::normalize(transform.apply(glm::vec3(0, 0, -1))); }

    glm::vec3 center() { return transform.position(); }

    void uv(glm::vec3 &u, glm::vec3 &v) {
        glm::cross(glm::vec3(0), glm::vec3(0));
        glm::vec3 first = transform.apply(mesh->getVertex(0));
        glm::vec3 second = transform.apply(mesh->getVertex(1));
        glm::vec3 third = transform.apply(mesh->getVertex(2));
        u = glm::normalize(second - first);
        v = glm::normalize(third - first);
    }

  private:
    void init(std::string n, float width, float height, glm::vec3 color) {
        name = n;
        type = "plane";
        material = new Material();
        material->colorDiffuse = color;

        mesh->addVertex(glm::vec3(-1, -1, 0), color);
        mesh->addVertex(glm::vec3(-1, 1, 0), color);
        mesh->addVertex(glm::vec3(1, -1, 0), color);
        mesh->addVertex(glm::vec3(1, 1, 0), color);

        mesh->addIndices(0, 1, 2);
        mesh->addIndices(1, 3, 2);

        mesh->addNormal(glm::vec3(0, 0, -1));
        mesh->addNormal(glm::vec3(0, 0, -1));
        mesh->addNormal(glm::vec3(0, 0, -1));
        mesh->addNormal(glm::vec3(0, 0, -1));

        transform.rotate(glm::vec3(1, 0, 0), 90.0f);
        transform.scale(glm::vec3(width, height, 1));

        mesh->commit();
    }
};
