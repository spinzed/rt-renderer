#include "renderer/Renderer.h"

#include "core/ParticleSystem.h"
#include "models/Mesh.h"
#include "models/Raster.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"
#include "utils/GLDebug.h"
#include "utils/ThreadPool.h"
#include "utils/mtr.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>

#define SKYBOX_COLOR glm::vec3(0.3, 0.4, 1)

#define RENDER_SHADOWMAPS 1

void Renderer::Init(int width, int height) {
    _width = width;
    _height = height;

    setDepth(RAYTRACE_DEPTH);
    setRenderingMethod(Rasterize);

    _clearColor = SKYBOX_COLOR;

    // OpenGL settings
    GLCheckError();
    glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], 1);

    glEnable(GL_DEPTH_TEST); // z buffer
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glEnable(GL_CULL_FACE); // culling

    glEnable(GL_PROGRAM_POINT_SIZE); // point rendering
    GLCheckError();

    glEnable(GL_DEBUG_OUTPUT); // debugging
    glDebugMessageCallback(debugCallback, nullptr);

    outputTexture = new Texture(GL_TEXTURE_2D, width, height);
    textureShower = new FullscreenTexture("tekstura", "texture"); // ili depthMapTexture
    textureShower->setTexture(outputTexture);

    depthFramebuffer = new Framebuffer();

    int RASTER_NUM = 2;

    for (int i = 0; i < RASTER_NUM; i++) {
        Raster<float> *r = new Raster<float>(width, height);
        rasteri.push_back(r);
    }

    lightMapShader = Shader::Load("pointLight");
    rt = Shader::LoadCompute("raytrace");
}

void Renderer::Render(RenderData data) {
    switch (method) {
    case Rasterize:
        rasterize(data);
        break;
    case Raycast:
    case Raytrace:
    case Pathtrace:
        rayRender(data);
        break;
    case Noop:
        break;
    default:
        assert(method);
    }
}

void Renderer::line(RenderData data, glm::vec3 current, glm::vec3 dx, glm::vec3 dy, int i) {
    glm::vec3 boja, target;
    float offsetx, offsety;

    glm::vec3 camPos = data.camera->position();

    for (int j = 0; j < _width; j++) {
        target = current;
        switch (method) {
        case Raycast:
            boja = raycast(data, camPos, target - camPos);
            break;
        case Raytrace:
            boja = raytrace(data, camPos, target - camPos, getDepth());
            break;
        case Pathtrace:
            offsetx = ((double)rand() / (RAND_MAX));
            offsety = ((double)rand() / (RAND_MAX));
            target = current + dx * offsetx + dy * offsety;
            boja = pathtrace(data, camPos, target - camPos, getDepth());
            break;
        default:
            std::runtime_error("unknown renderer type");
        }
        rasteri[currentRasterIndex]->setFragmentColor(j, i, boja);
        current += dx;
    }
}

void Renderer::rayRender(RenderData data) {
    t.reset();
    Camera *camera = data.camera;

    glm::vec3 camPos = camera->position();
    CameraConstraints c = camera->constraints;

    glm::vec3 start = camPos + c.nearPlane * camera->forward() + c.top * camera->up() + c.left * camera->right();
    glm::vec3 current = start;
    glm::vec3 row = (c.right - c.left) * camera->right();
    glm::vec3 dx = row * (1.0f / _width);
    glm::vec3 column = -(c.top - c.bottom) * camera->up();
    glm::vec3 dy = column * (1.0f / _height);

    textureShower->shader->use();

#if RAYTRACE_MULTICORE
    if (!pool)
        pool = new ThreadPool();

    pool->setJobQueue(_height);
    for (int i = 0; i < _height; i++) {
        current = start + column * ((float)i / (_height - 1));
        // pool->enqueue(&Renderer::line, nullptr, current, dx, dy, i);
        pool->enqueue([data, current, dx, dy, i] { line(data, current, dx, dy, i); });
    }

    pool->wait();
#endif
#if !RAYTRACE_MULTICORE
    for (int i = 0; i < _height; i++) {
        current = start + column * ((float)i / (_height - 1));
        line(current, dx, dy, i);
    }
#endif

    // rt->compute(_width, _height);
    // textureShower->setTexture(tx);
    // textureShower->render();

    std::cout << "Render done, number of renders: " << ++renderCount << std::endl;
    t.printElapsed("Time elapsed since last render: ");
    totalTime += t.elapsed();
    std::cout << "Total elapsed time rendering:   " << totalTime << "ms" << std::endl;

    if (monteCarlo) {
        Raster<float> *current = rasteri[currentRasterIndex];
        Raster<float> *other = rasteri[!currentRasterIndex];
        for (int i = 0; i < _height; i++) {
            for (int j = 0; j < _width; j++) {
                glm::vec3 c1 = current->getFragmentColor(j, i);
                glm::vec3 c2 = other->getFragmentColor(j, i);
                glm::vec3 color = (c1 + (c2 * (float)(renderCount - 1))) * (1.0f / renderCount);
                rasteri[currentRasterIndex]->setFragmentColor(j, i, color);
            }
        }
    }

    iscrtajRaster();
    currentRasterIndex = !currentRasterIndex;

    _cameraMatrixChanged = false;
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
            lightMapShader->setUniform(SHADER_MMATRIX, 1, o->getModelMatrix());
            o->render(lightMapShader);
        }
        for (Object *child : o->children) {
            if (o->mesh != nullptr && child->mesh->getPrimitiveType() == GL_TRIANGLES) {
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
            shader->setTexture(SHADER_TEXTURE, 0, m->texture);
        }
        shader->setUniform(SHADER_HAS_TEXTURES, m->texture > 0);
    }

    shader->setTexture(SHADER_SHADOWMAP, 1, outputTexture->id);
    shader->setUniform(SHADER_HAS_SHADOWMAP, RENDER_SHADOWMAPS);

    if (light) {
        light->cb.use(3);
        shader->setUniform(SHADER_SHADOWMAPCUBE, 3);
    }
    shader->setUniform(SHADER_HAS_SHADOWMAPCUBE, light != nullptr && RENDER_SHADOWMAPS);

    if (data.skybox != nullptr) {
        data.skybox->cubemap->use(2);
        shader->setUniform(SHADER_SKYBOX, 2);
    }
    shader->setUniform(SHADER_HAS_SKYBOX, data.skybox != nullptr);
}

glm::vec3 calculateLight(Light *l, const glm::vec3 &normal, const glm::vec3 shadingPoint, const glm::vec3 &cameraPos) {
    glm::vec3 lpos = l->getTransform()->position();
    glm::vec3 lightDir = glm::normalize(lpos - shadingPoint);
    glm::vec3 cameraDir = glm::normalize(cameraPos - shadingPoint);

    // diffuse
    float diffuseStrength = std::max(0.0f, glm::dot(lightDir, normal));

    // specular
    glm::vec3 reflected = glm::normalize(glm::reflect(-lightDir, normal));
    float specularBase = std::max(0.0f, glm::dot(cameraDir, reflected));
    float specularStrength = glm::pow(specularBase, 32);

    float d = glm::distance(lpos, shadingPoint);
    float i = glm::max((l->range - d) / l->range, 0.0f);

    return l->color * (diffuseStrength + specularStrength) * l->intensity * i;
}

glm::vec3 Renderer::phong(Intersection &p, glm::vec3 diffuseColor, RenderData data) {
    glm::vec3 light = diffuseColor;

    if (data.lights->empty())
        return light;

    Light *l = data.lights->at(0);

    Object *o = nullptr;
    std::optional<Intersection> p2 = raycast(data, p.point, l->getTransform()->position() - p.point, o); // shadow ray

    if (!p2.has_value() || p2.value().t > 1) {
        glm::vec3 c = calculateLight(l, p.normal, p.point, data.camera->position());
        light += c;
    }

    return light * p.color;
}

std::optional<Intersection> Renderer::raycast(RenderData data, glm::vec3 origin, glm::vec3 direction,
                                              Object *&intersectedObject) {
    Intersection intersect;
    bool found = false;

    for (Object *o : *data.objects) {
        std::optional<Intersection> p = o->findIntersection(origin, direction);
        if (!p.has_value()) {
            continue;
        }
        if (!found || (p.value().t < intersect.t && p.value().t > 1e-5)) {
            intersect = p.value();
            intersectedObject = o;
            found = true;
        }
    }
    if (!found)
        return std::nullopt;

    return intersect;
}

glm::vec3 Renderer::raycast(RenderData data, glm::vec3 origin, glm::vec3 direction) {
    // Object *intersectedObject = nullptr;
    // IntersectPoint intersect = raycast(origin, direction, intersectedObject);
    // return intersectedObject ? phong(intersect, glm::vec3(0.2, 0.2, 0.2)) : _clearColor;
    return raytrace(data, origin, direction, 1);
}

int test = 1;
glm::vec3 Renderer::raytrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth) {
    if (depth == 0)
        return glm::vec3(0);

    Object *object = nullptr;
    std::optional<Intersection> intersection = raycast(data, origin, direction, object);
    if (!intersection.has_value())
        return _clearColor;

    Intersection p = intersection.value();

    glm::vec3 light = RAYTRACE_AMBIENT;
    glm::vec3 normal = p.normal;
    glm::vec3 color = p.color * light + phong(p, glm::vec3(0), data);

    bool hasReflective = object->mesh->material && object->mesh->material->colorReflective != glm::vec3(0);
    glm::vec3 reflectiveMat = hasReflective ? object->mesh->material->colorReflective : glm::vec3(k_specular);
    if (reflectiveMat != glm::vec3(0) && depth > 1) {
        glm::vec3 rayColor = raytrace(data, p.point, glm::reflect(direction, normal), depth - 1);
        color = glm::lerp(color, reflectiveMat * rayColor, reflectiveMat);
    }
    bool hasTransmitive = object->mesh->material && object->mesh->material->colorTransmitive != glm::vec3(0);
    glm::vec3 transmitiveMat = hasTransmitive ? object->mesh->material->colorTransmitive : glm::vec3(k_transmit);
    if (transmitiveMat != glm::vec3(0) && depth > 1) {
        float eta = 1.0f;
        glm::vec3 refractedDir = glm::refract(direction, normal, eta);
        color =
            glm::lerp(color, raytrace(data, p.point + 0.001f * refractedDir, refractedDir, depth - 1), transmitiveMat);
    }

    return color;
}

glm::vec3 Renderer::pathtrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth) {
    if (depth == 0)
        return glm::vec3(0);

    Object *object = nullptr;
    std::optional<Intersection> intersection = raycast(data, origin, direction, object);
    if (!intersection.has_value())
        return _clearColor;

    Intersection p = intersection.value();

    glm::vec3 light = RAYTRACE_AMBIENT;
    glm::vec3 normal = p.normal;
    glm::vec3 color = p.color * light + phong(p, glm::vec3(0), data);

    bool hasReflective = object->mesh->material && object->mesh->material->colorReflective != glm::vec3(0);
    glm::vec3 reflectiveMat = hasReflective ? object->mesh->material->colorReflective : glm::vec3(k_specular);
    if (reflectiveMat != glm::vec3(0) && depth > 1) {
        glm::vec3 randomDirection = glm::reflect(direction, normal + k_roughness * mtr::linearRandVec3(-0.5f, 0.5f));
        glm::vec3 rayColor = pathtrace(data, p.point, randomDirection, depth - 1);
        color = glm::lerp(color, reflectiveMat * rayColor, reflectiveMat);
    }
    bool hasTransmitive = object->mesh->material && object->mesh->material->colorTransmitive != glm::vec3(0);
    glm::vec3 transmitiveMat = hasTransmitive ? object->mesh->material->colorTransmitive : glm::vec3(k_transmit);
    if (transmitiveMat != glm::vec3(0) && depth > 1) {
        float eta = 1.0f;
        glm::vec3 refractedDir = glm::refract(direction, normal + k_roughness * mtr::linearRandVec3(-0.5f, 0.5f), eta);
        color =
            glm::lerp(color, raytrace(data, p.point + 0.001f * refractedDir, refractedDir, depth - 1), transmitiveMat);
    }

    return color;
}

void Renderer::iscrtajRaster() {
    // rasteri[currentRasterIndex]->setFragmentColor(0, rasteri[currentRasterIndex]->height - 1, glm::vec3(69));
    textureShower->loadRaster(rasteri[currentRasterIndex]);
    textureShower->render();
}

void Renderer::spremiRaster() {
    int *buffer = new int[_width * _height * 3];
    glReadPixels(0, 0, _width, _height, GL_BGR, GL_UNSIGNED_BYTE, buffer);
}

void Renderer::SetResolution(int width, int height) {
    _width = width;
    _height = height;
    glViewport(0, 0, width, height);
    if (outputTexture != nullptr) {
        outputTexture->setSize(width, height);
    }
    for (Raster<float> *r : rasteri) {
        r->resize(width, height);
    }
}
