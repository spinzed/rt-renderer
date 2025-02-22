#include "renderer/Renderer.h"

#include "core/ParticleSystem.h"
#include "models/Mesh.h"
#include "models/Raster.h"
#include "objects/Object.h"
#include "renderer/Raytracer.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"
#include "utils/GLDebug.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/string_cast.hpp>

#define SKYBOX_COLOR glm::vec3(0.3, 0.4, 1)

#define RENDER_SHADOWMAPS 1

void Renderer::Init(int width, int height) {
    _width = width;
    _height = height;

    _clearColor = SKYBOX_COLOR;

    // OpenGL settings
    GLCheckError();

    glEnable(GL_DEBUG_OUTPUT); // debugging
    glDebugMessageCallback(debugCallback, nullptr);

    GLCheckError();
    glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], 1);

    glEnable(GL_DEPTH_TEST); // z buffer
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glEnable(GL_CULL_FACE); // culling

    glEnable(GL_PROGRAM_POINT_SIZE); // point rendering

    GLCheckError();

    outputTexture = new Texture(GL_TEXTURE_2D, width, height); // for RT renders
    textureShower = new FullscreenTexture("tekstura", "texture");
    textureShower->setTexture(outputTexture);

    // depthFramebuffer = new Framebuffer();

    lightMapShader = Shader::Load("pointLight");
    rt = Shader::LoadCompute("raytrace");

    raytracer.Init(width, height);
}

void Renderer::Render(RenderData data) {
    switch (method) {
    case Rasterize:
        rasterize(data);
        break;
    case Raycast:
    case Raytrace:
    case Pathtrace:
        raytracer.Render(data, textureShower);
        textureShower->render();
        _cameraMatrixChanged = false;
        break;
    case Noop:
        break;
    default:
        assert(method);
    }
}

void Renderer::rasterize(RenderData data) {
    // ugly, but will make do (for now)
    lightPositions.clear();
    lightIntensities.clear();
    lightColors.clear();

    for (const auto &l : *data.lights) {
        glm::vec3 pos = l->getTransform()->position();
        lightPositions.insert(lightPositions.end(), {pos[0], pos[1], pos[2]});
        lightIntensities.insert(lightIntensities.end(), {l->intensity[0], l->intensity[1], l->intensity[2]});
        lightColors.insert(lightColors.end(), {l->color[0], l->color[1], l->color[2]});

        PointLight *light = dynamic_cast<PointLight *>(l);
        assert(light != nullptr);
        depthFramebuffer->setDepthTexture(&light->cb); // enough to be ran only once
    }

    if (light != nullptr) {
        depthFramebuffer->use();
        depthFramebuffer->setupDepth();
        lightMapShader->use();
        lightMapShader->setFloat("farPlane", light->farPlane);
        lightMapShader->setMatrices("shadowMatrices", light->transforms);
        light->cb.use(0);
        lightMapShader->setVector("lightPos", light->getTransform()->position());
    }

    // commit uncommited objects before rendering
    for (Object *o : *data.objects) {
        if (o->uncommited)
            o->commit(true);

        for (Object *child : o->children) {
            if (child->uncommited)
                child->commit(true);
        }
    }

    for (ParticleCluster *pc : ParticleSystem::clusters) {
        if (pc->uncommited)
            pc->commit(true);

        for (Object *child : pc->children) {
            if (child->uncommited)
                child->commit(true);
        }
    }

    // 1st pass - depth
    for (Object *o : *data.objects) {
        if (o->mesh != nullptr && o->mesh->getPrimitiveType() == GL_TRIANGLES) {
            lightMapShader->use();
            lightMapShader->setMatrix("mMatrix", o->getModelMatrix());
            o->render(lightMapShader);
        }
        for (Object *child : o->children) {
            if (child->mesh != nullptr && child->mesh->getPrimitiveType() == GL_TRIANGLES) {
                lightMapShader->use();
                lightMapShader->setUniform(SHADER_MMATRIX, 1, child->getModelMatrix());
                child->render(lightMapShader);
            }
        }
    }
    depthFramebuffer->cleanDepth(_width, _height);

    // 2nd pass - scene with shadows
    if (data.skybox) {
        UpdateShader(data.skybox, data);
        data.skybox->render();
    }
    for (Object *o : *data.objects) {
        // TODO: remove updateShader and put it as the object method that recieves renderer state
        // and sets the uniform vars itself
        UpdateShader(o, data);
        o->render();
        for (Object *o2 : o->children) {
            UpdateShader(o2, data);
            o2->render();
        }
    }
    for (ParticleCluster *pc : ParticleSystem::clusters) {
        UpdateShader(pc, data);
        pc->render();
    }
}

void Renderer::UpdateShader(Object *object, RenderData data) {
    Shader *shader = object->shader;
    if (shader == nullptr)
        return;

    glm::vec3 cameraPos = data.camera->position();
    Transform viewTransform(data.camera->getViewMatrix());
    glm::mat4 projMat = data.camera->getProjectionMatrix();

    shader->use();

    shader->setUniform(SHADER_MMATRIX, 1, object->getModelMatrix());
    shader->setUniform(SHADER_PVMATRIX, 1, projMat * viewTransform.matrix);
    viewTransform.setPosition(glm::vec3(0));
    glm::mat4 centered = projMat * viewTransform.matrix;
    shader->setUniform(SHADER_PVCENTERMATRIX, 1, centered);
    shader->setUniform(SHADER_CAMERA, 1, cameraPos);

    shader->setUniform(SHADER_LIGHT_NUM, int(lightPositions.size() / 3));
    shader->setUniform(SHADER_LIGHT_POSITION, lightPositions.size() / 3, lightPositions);
    shader->setUniform(SHADER_LIGHT_INTENSITY, lightIntensities.size() / 3, lightIntensities);
    shader->setUniform(SHADER_LIGHT_COLOR, lightColors.size() / 3, lightColors);

    shader->setFloat("range", light ? light->farPlane : 10);

    if (object->mesh && object->mesh->material) {
        Material *m = object->mesh->material;
        shader->setUniform(SHADER_MATERIAL_COLOR_AMBIENT, 1, m->colorAmbient);
        shader->setUniform(SHADER_MATERIAL_COLOR_DIFFUSE, 1, m->colorDiffuse);
        shader->setUniform(SHADER_MATERIAL_COLOR_SPECULAR, 1, m->colorSpecular);
        shader->setUniform(SHADER_MATERIAL_SHININESS, m->shininess);
        shader->setUniform(SHADER_MATERIAL_COLOR_REFLECTIVE, 1, m->colorReflective);
        shader->setUniform(SHADER_MATERIAL_COLOR_EMISSIVE, 1, m->colorEmissive);

        if (m->texture > 0) {
            shader->setTexture(SHADER_TEXTURE, 3, m->texture);
        }
        shader->setUniform(SHADER_HAS_TEXTURES, m->texture > 0);
    }

    shader->setTexture(SHADER_SHADOWMAP, RENDER_SHADOWMAPS ? 4 : 0, outputTexture->id);
    shader->setUniform(SHADER_HAS_SHADOWMAP, RENDER_SHADOWMAPS);

    if (light) {
        light->cb.use(5);
    }
    shader->setUniform(SHADER_SHADOWMAPCUBE, light != nullptr && RENDER_SHADOWMAPS ? 5 : 1);
    shader->setUniform(SHADER_HAS_SHADOWMAPCUBE, light != nullptr && RENDER_SHADOWMAPS);

    if (data.skybox != nullptr) {
        data.skybox->cubemap->use(2);
    }
    shader->setUniform(SHADER_SKYBOX, data.skybox ? 2 : 1);
    shader->setUniform(SHADER_HAS_SKYBOX, data.skybox != nullptr);
}

void Renderer::SetResolution(int width, int height) {
    _width = width;
    _height = height;
    glViewport(0, 0, width, height);
    if (outputTexture != nullptr) {
        outputTexture->setSize(width, height);
    }
    raytracer.SetResolution(width, height);
}
