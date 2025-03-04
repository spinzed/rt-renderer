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

    glm::vec3 normal() { return glm::normalize(glm::vec3(transform.getMatrix() * glm::vec4(0, 0, 1, 1))); }

    glm::vec3 center() { return transform.position(); }

    void uv(glm::vec3 &u, glm::vec3 &v) {
        glm::vec3 first = mesh->getVertex(0);
        u = mesh->getVertex(1) - first;
        v = mesh->getVertex(2) - first;
    }

    glm::vec3 color;

  private:
    void init(std::string name, float width, float height, glm::vec3 color) {
        this->name = name;
        this->color = color;
        type = "plane";

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
