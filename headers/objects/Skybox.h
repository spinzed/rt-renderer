#pragma once

#include "models/Mesh.h"
#include "objects/Object.h"
#include "renderables/MeshRenderer.h"
#include "renderer/Cubemap.h"

#include <string>

class Skybox : public Object {
  public:
    Skybox(Cubemap *cm) : Object("skybox", "skybox") {
        cubemap = cm;
        shader = Shader::Load("skybox");

        mesh = new Mesh(GL_TRIANGLES);
        mesh->addVertex(-1, -1, -1); // v0
        mesh->addVertex(-1, -1, 1); // v1
        mesh->addVertex(-1, 1, -1); // v2
        mesh->addVertex(-1, 1, 1); // v3
        mesh->addVertex(1, -1, -1); // v4
        mesh->addVertex(1, -1, 1); // v5
        mesh->addVertex(1, 1, -1); // v6
        mesh->addVertex(1, 1, 1); // v7

        // note: indices go in the other direction so that the culling happens when
        // the viewer is outside the cube, not inside

        // Front face
        mesh->addIndices(0, 2, 1);
        mesh->addIndices(1, 2, 3);

        // Back face
        mesh->addIndices(4, 5, 6);
        mesh->addIndices(5, 7, 6);

        // Left face
        mesh->addIndices(0, 4, 2);
        mesh->addIndices(2, 4, 6);

        // Right face
        mesh->addIndices(1, 3, 5);
        mesh->addIndices(5, 3, 7);

        // Top face
        mesh->addIndices(2, 6, 3);
        mesh->addIndices(3, 6, 7);

        // Bottom face
        mesh->addIndices(0, 1, 4);
        mesh->addIndices(1, 5, 4);

        renderable = new MeshRenderer(mesh);

        //cubemap->use(0);
    }

    virtual void render() {
        glDepthMask(GL_FALSE);
        Object::render();
        glDepthMask(GL_TRUE);
    }

    Cubemap *cubemap = nullptr;
};