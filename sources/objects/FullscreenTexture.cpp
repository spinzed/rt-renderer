#include "objects/FullscreenTexture.h"

#include "renderables/MeshRenderer.h"
#include "renderer/Shader.h"

FullscreenTexture::FullscreenTexture(std::string name, std::string shaderName) : Object(name, "texture") {
    shader = Shader::Load(shaderName);

    mesh = new Mesh(GL_TRIANGLES);
    mesh->addVertex(-1, -1, 0, 0, 0, 1);
    mesh->addVertex(1, -1, 0, 1, 0, 1);
    mesh->addVertex(1, 1, 0, 1, 1, 1);
    mesh->addVertex(-1, 1, 0, 0, 1, 1);
    mesh->addIndices(0, 1, 2);
    mesh->addIndices(0, 2, 3);
    mesh->commit();

    renderable = new MeshRenderer(mesh);
}

FullscreenTexture::~FullscreenTexture() { delete mesh; }

void FullscreenTexture::setTexture(Texture *t) {
    texture = t;
    shader->use();
    shader->setTexture(SHADER_TEXTURE, 0, texture->id);
    // shader->setUniform(SHADER_TEXTURE, 0);
}
