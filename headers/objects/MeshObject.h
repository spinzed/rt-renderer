#pragma once

#include "models/Mesh.h"
#include "objects/Object.h"
#include "renderables/MeshRenderer.h"
#include "renderer/Shader.h"

#include <iostream>

class MeshObject : public Object {
  public:
    MeshObject(std::string name, Mesh *mesh) : Object(name, "mesh") { Init(mesh, Shader::Load("phong")); }
    MeshObject(std::string name, Mesh *mesh, Shader *shader) : Object(name, "mesh") { Init(mesh, shader); }

    // todo: finish
    MeshObject *Load(std::string resourceName, std::string objectName) {
        std::string error;
        const aiScene *scene = Loader::LoadResource(resourceName, error);
        if (!scene) {
            std::cout << "Error importing " << resourceName << ": " << error << std::endl;
            return nullptr;
        }
        return new MeshObject(objectName, nullptr);
    }

  private:
    void Init(Mesh *mesh, Shader *shader) {
        this->mesh = mesh;
        this->renderable = new MeshRenderer(mesh);
        this->shader = shader;
    }
};
