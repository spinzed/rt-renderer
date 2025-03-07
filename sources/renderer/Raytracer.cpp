#include "renderer/Raytracer.h"

#include "objects/FullscreenTexture.h"
#include "renderer/Raytracer.cuh"
#include "utils/mtr.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

#include <iostream>

void Raytracer::Init(int w, int h) {
    width = w;
    height = h;

    int RASTER_NUM = 2;

    for (int i = 0; i < RASTER_NUM; i++) {
        Raster<float> *r = new Raster<float>(width, height);
        rasteri.push_back(r);
    }

#if ENABLE_CUDA
    CudaRT::init();
#endif
}

void Raytracer::Render(RenderData data, FullscreenTexture *output) {
    assert(width > 0 && height > 0);
    debugString = "";

    t.reset();

#if ENABLE_CUDA
    HardwareRender(data);
    debugString += CudaRT::debugString;
#else
    SoftwareRender(data);
#endif

    // t.printFormatted("Render Time: ");
    debugString += t.format("Render Time: $\n");
    // std::cout << "Render done, number of renders: " << ++renderCount << std::endl;
    // totalTime += t.elapsed();
    // std::cout << "Total Render Time: " << totalTime << "ms" << std::endl;
    renderCount++;

    if (monteCarlo) {
#if ENABLE_CUDA
        CudaRT::monteCarlo(width, height, renderCount, InactiveRaster()->get(), CurrentRaster()->get());
#else
        MonteCarlo();
#endif
        debugString += t.format(std::format("Monte Carlo after {} interations: # ($)\n", renderCount));
    }

    output->loadRaster(monteCarlo ? InactiveRaster() : CurrentRaster());
    // SwitchRaster();
    debugString += t.format("Raster Load: # ($)\n");
}

// #if ENABLE_CUDA
// struct RTObject;
//
// void launchAddKernel(int *a, int *b, int *c, int size);
// #endif

void Raytracer::HardwareRender(RenderData data) {
#if ENABLE_CUDA
    CudaRT::render(width, height, depth, data, CurrentRaster()->get());
#endif
}

// deprecated
void Raytracer::SoftwareRender(RenderData data) {
    Camera *camera = data.camera;

    glm::vec3 camPos = camera->position();
    CameraConstraints c = camera->constraints;

    glm::vec3 start = camPos + c.nearPlane * camera->forward() + c.top * camera->up() + c.left * camera->right();
    glm::vec3 current = start;
    glm::vec3 row = (c.right - c.left) * camera->right();
    glm::vec3 dx = row * (1.0f / width);
    glm::vec3 column = -(c.top - c.bottom) * camera->up();
    glm::vec3 dy = column * (1.0f / height);

#if RAYTRACE_MULTICORE
    if (!pool)
        pool = new ThreadPool();

    pool->setJobQueue(height);
    for (int i = 0; i < height; i++) {
        current = start + column * ((float)i / (height - 1));
        pool->enqueue([data, current, dx, dy, i, this] { line(data, current, dx, dy, i); });
    }

    pool->wait();
#endif
#if !RAYTRACE_MULTICORE
    for (int i = 0; i < height; i++) {
        current = start + column * ((float)i / (height - 1));
        line(current, dx, dy, i);
    }
#endif
}

void Raytracer::MonteCarlo() {
    Raster<float> *current = rasteri[currentRasterIndex];
    Raster<float> *other = rasteri[!currentRasterIndex];
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            glm::vec3 c1 = current->getFragmentColor(j, i);
            glm::vec3 c2 = other->getFragmentColor(j, i);
            glm::vec3 color = (c1 + (c2 * (float)(renderCount - 1))) * (1.0f / renderCount);
            rasteri[currentRasterIndex]->setFragmentColor(j, i, color);
        }
    }
}

void Raytracer::SetResolution(int w, int h) {
    width = w;
    height = h;
    for (Raster<float> *r : rasteri) {
        r->resize(width, height);
    }
}

void Raytracer::line(RenderData data, glm::vec3 current, glm::vec3 dx, glm::vec3 dy, int i) {
    glm::vec3 boja, target;
    float offsetx, offsety;

    glm::vec3 camPos = data.camera->position();

    for (int j = 0; j < width; j++) {
        target = current;
        offsetx = ((double)rand() / (RAND_MAX));
        offsety = ((double)rand() / (RAND_MAX));
        target = current + dx * offsetx + dy * offsety;
        boja = pathtrace(data, camPos, target - camPos, getDepth());

        rasteri[currentRasterIndex]->setFragmentColor(j, i, boja);
        current += dx;
    }
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

glm::vec3 Raytracer::phong(Intersection &p, glm::vec3 diffuseColor, RenderData data) {
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

std::optional<Intersection> Raytracer::raycast(RenderData data, glm::vec3 origin, glm::vec3 direction,
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

glm::vec3 Raytracer::raycast(RenderData data, glm::vec3 origin, glm::vec3 direction) {
    // Object *intersectedObject = nullptr;
    // IntersectPoint intersect = raycast(origin, direction, intersectedObject);
    // return intersectedObject ? phong(intersect, glm::vec3(0.2, 0.2, 0.2)) : _clearColor;
    return raytrace(data, origin, direction, 1);
}

int test = 1;
glm::vec3 Raytracer::pathtrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth) {
    return raytrace(data, origin, direction, depth);
}

glm::vec3 Raytracer::raytrace(RenderData data, glm::vec3 origin, glm::vec3 direction, int depth) {
    if (depth == 0)
        return glm::vec3(0);

    Object *object = nullptr;
    std::optional<Intersection> intersection = raycast(data, origin, direction, object);
    if (!intersection.has_value())
        return clearColor;

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

void Raytracer::resetStats() {
    renderCount = 0;
    totalTime = 0;
}

Raster<float> *Raytracer::CurrentRaster() { return rasteri[currentRasterIndex]; }
Raster<float> *Raytracer::InactiveRaster() { return rasteri[!currentRasterIndex]; }
void Raytracer::SwitchRaster() { currentRasterIndex = !currentRasterIndex; }
