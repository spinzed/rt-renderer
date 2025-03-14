#include "objects/Sphere.h"

#include <glm/glm.hpp>

#include "utils/mtr.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Sphere::Sphere(std::string name, glm::vec3 center, float radius, glm::vec3 color)
    : MeshObject(name, new Mesh(GL_TRIANGLES), Shader::Load("phong")) {
    init(name, center, radius, color);
}

Sphere::Sphere(std::string name, glm::vec3 center, float radius)
    : MeshObject(name, new Mesh(GL_TRIANGLES), Shader::Load("phong")) {
    init(name, center, radius, glm::vec3(1));
}

void Sphere::init(std::string n, glm::vec3 center, float radius, glm::vec3 c) {
    name = n;
    type = "sphere";
    transform.translate(center);
    transform.scale(radius);
    material = new Material();
    material->colorDiffuse = c;

    generateSphere();
    mesh->commit();
}

inline glm::vec3 &Sphere::color() { return material->colorDiffuse; }

Sphere::~Sphere() { delete mesh; }

std::optional<Intersection> Sphere::findIntersection(glm::vec3 origin, glm::vec3 direction) {
    Transform t = getFullTransform();
    glm::vec3 center = t.position();
    float radius = t.getScale().x;

    float t1, t2;
    unsigned int koliko = mtr::intersectLineAndSphere(origin, direction, center, radius, t1, t2);
    if (koliko == 2 && t2 > 0 && t2 < t1) {
        glm::vec3 point = origin + t2 * direction;
        return Intersection(t2, point, color(), glm::normalize(point - center));
    }
    if (koliko > 0 && t1 > 0) {
        glm::vec3 point = origin + t1 * direction;
        return Intersection(t1, point, color(), glm::normalize(point - center));
    }
    return std::nullopt;
}

const int STACKS = 30;
const int SLICES = 30;

// za jedinicnu kuglu
void Sphere::generateSphere() {
    for (int i = 0; i <= STACKS; ++i) {
        float V = (float)i / (float)STACKS;
        float phi = V * M_PI;

        for (int j = 0; j <= SLICES; ++j) {
            float U = (float)j / (float)SLICES;
            float theta = U * (M_PI * 2.0f);

            glm::vec3 unit(cosf(theta) * sinf(phi), cosf(phi), sinf(theta) * sinf(phi));

            mesh->addVertex(unit, color()); // center + unit * radius
            mesh->addNormal(unit);
        }
    }

    for (int i = 0; i < SLICES * STACKS + SLICES; ++i) {
        mesh->addIndices(i, i + SLICES + 1, i + SLICES);
        mesh->addIndices(i + SLICES + 1, i, i + 1);
    }
    mesh->addIndices(1, 2, 0);
}
