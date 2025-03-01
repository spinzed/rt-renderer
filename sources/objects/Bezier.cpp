#include "objects/Bezier.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <memory>

#define SUBDIVISIONS 100

Bezier::Bezier() : Object("bezier", "line") {
    shader = Shader::Load("line");

    approxCurve = std::make_shared<PolyLine>();
    interpCurve = std::make_shared<PolyLine>();
    controlMesh = std::make_shared<Mesh>(GL_TRIANGLES);
    controlPolygon = std::make_shared<MeshObject>("controlPolygon", controlMesh.get(), shader);

    // this should be refractored
    approxCurve->shader = shader;
    interpCurve->shader = shader;
}

void Bezier::addControlPoint(glm::vec3 point) {
    glm::vec3 color(1, 0, 0);
    points.push_back(point);

    controlPolygon->mesh->addVertex(point, color);

    int n = controlPolygon->mesh->numberOfVertices();
    if (n >= 2) {
        controlPolygon->mesh->addIndices(n - 2, n - 1);
    }
};

void Bezier::finish() {
    approxCurve->reset();
    interpCurve->reset();

    constructApproxCurve();
    constructInterpCurve();

    approxCurve->commit();
    interpCurve->commit();
    controlPolygon->mesh->commit();
}

void Bezier::constructApproxCurve() {
    for (int t = 0; t <= SUBDIVISIONS; t++) {
        glm::vec3 point = evaluateApproxPoint((float)t / SUBDIVISIONS);
        // mesh->addVertexStrip(point, color);
        approxCurve->addPoint(point);
    }
}

void Bezier::constructInterpCurve() {
    int n = points.size();
    if (n < 4)
        return;

    glm::mat4 A(18.0, -33.0, 21.0, -6.0, 0.0, 54.0, -81.0, 27.0, 0.0, -27.0, 81.0, -54.0, 0.0, 6.0, -21.0, 33.0);

    glm::mat4 P(glm::vec4(points[n - 4], 1.0), glm::vec4(points[n - 3], 1.0), glm::vec4(points[n - 2], 1.0),
                glm::vec4(points[n - 1], 1.0));

    // glm::mat4 aComp = ((1.0f / 18.0f) * glm::transpose(A) * glm::transpose(P));
    aInterp = (1.0f / 18.0f) * P * glm::transpose(A);

    glm::vec3 color(1, 0, 1);

    glm::vec3 p;
    float t;

    for (int i = 0; i <= SUBDIVISIONS; i++) {
        t = (1.0f / SUBDIVISIONS) * i;
        p = evaluateInterpPoint(t);
        interpCurve->addPoint(p, color);
    }
}

void Bezier::render() {
    approxCurve->render();
    interpCurve->render();
    controlPolygon->render();
}

int fact(int n) {
    int r = 1;
    for (int i = 2; i <= n; i++) {
        r *= i;
    }
    return r;
}

glm::vec3 Bezier::evaluateApproxPoint(float t) {
    int n = degree();
    glm::vec3 r(0);

    float nfact = (float)fact(n);
    float koef = nfact, ti = 1, jmti = pow(1 - t, n);
    float dili = t == 1 ? 1 : (1 - t);

    for (int i = 0; i <= n; i++) {
        glm::vec3 tocka = points[i];
        koef = nfact / (fact(i) * fact(n - i));
        r += tocka * koef * ti * (i == n ? 1 : jmti);
        ti *= t;
        jmti /= dili;
    }
    return r;
}

glm::vec3 Bezier::evaluateInterpPoint(float t) {
    return aInterp[0] + aInterp[1] * (float)(3 * t - 3 * glm::pow(t, 2) + glm::pow(t, 3)) +
           aInterp[2] * (float)(3 * glm::pow(t, 2) - 2 * glm::pow(t, 3)) + aInterp[3] * (float)glm::pow(t, 3);
}
