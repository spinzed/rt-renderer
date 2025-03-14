#include "examples/ExampleRT.h"

// Local Headers
#include "core/Animation.h"
#include "core/Animator.h"
#include "core/Behavior.h"
#include "renderer/Camera.h"
#include "core/Input.h"
#include "core/Transform.h"
#include "core/UI.h"
#include "core/WindowManager.h"
#include "models/Mesh.h"
#include "objects/MeshObject.h"
#include "objects/PointCloud.h"
#include "objects/PolyLine.h"
#include "renderer/Cubemap.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "utils/ThreadPool.h"

// System Headers
#include "glm/common.hpp"
#include "imgui.h"
#include "utils/mtr.h"
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <objects/Plane.h>
#include <objects/Sphere.h>

void ExampleRT::cursorPositionCallback(WindowCursorEvent event) {
    if (!engine->manager->focused)
        return;

    int dx = -event.xpos;
    int dy = -event.ypos;
    Camera *camera = engine->GetCamera();

    camera->rotate(mouseSensitivity * dx, mouseSensitivity * dy);
    camera->recalculateMatrix();

    engine->manager->CenterCursor();
}

int ExampleRT::run(std::string execDirectory) {
    engine->Init(width, height, execDirectory);

    engine->manager->SetCursorMode(CursorMode::DISABLED);

    engine->manager->setCursorCallback([&](auto a) { cursorPositionCallback(a); });

    /*********************************************************************************************/
    Camera *camera = engine->GetCamera();
    camera->translate(glm::vec3(3.0f, 3.0f, -3.0f));
    camera->rotate(145, -30);
    camera->recalculateMatrix();

    Cubemap skybox = Cubemap::Load("skybox");
    engine->SetSkybox(&skybox);

    Mesh *mesh = Mesh::Load("stroj");
    MeshObject arwing("arwing", mesh);
    //.getTransform()->scale(glm::vec3(5, 1, 5));
    arwing.material = new Material();
    arwing.material->colorDiffuse = glm::vec3(1, 0.5, 0.0);
    arwing.getTransform()->translate(glm::vec3(5, 8, 0));
    arwing.getTransform()->scale(1);
    engine->AddObject(&arwing);

    Sphere s("sphere", glm::vec3(1, 0.8, 1), 3, glm::vec3(0, 1, 1));
    s.getTransform()->translate(glm::vec3(1, 5, 0));
    s.material->colorEmissive = glm::vec3(1, 1, 1);
    // s.material->emissiveStrength = 10;
    engine->AddObject(&s);

    Sphere s2("sphere", glm::vec3(1, 0.8, 1), 3, glm::vec3(0, 1, 1));
    s2.getTransform()->translate(glm::vec3(1, 1, 1));
    s2.material->smoothness = 0.4f;
    engine->AddObject(&s2);

    Sphere s3("sphere", glm::vec3(1, 0.8, 1), 3, glm::vec3(0, 1, 1));
    s3.getTransform()->translate(glm::vec3(-1, 4, 0));
    s3.material->smoothness = 1;
    engine->AddObject(&s3);

    Plane p("plane", 10, 10, glm::vec3(0.8, 0, 0.1));
    engine->AddObject(&p);
    Plane zid1("plane", 1, 1, glm::vec3(0.5, 0.5, 0));
    zid1.getTransform()->rotateCenter(TransformIdentity::right(), 90.0f);
    zid1.getTransform()->scale(10);
    zid1.getTransform()->translate(glm::vec3(0, -1, 1));
    engine->AddObject(&zid1);
    Plane zid2("plane", 1, 1, glm::vec3(1, 1, 1));
    zid2.getTransform()->rotateCenter(TransformIdentity::up(), 90.0f);
    zid2.getTransform()->scale(10);
    zid2.getTransform()->translate(glm::vec3(1, 0, 1));
    // zid2.material->smoothness = 1;
    engine->AddObject(&zid2);
    Plane zid3("plane", 1, 1, glm::vec3(0, 0, 1));
    zid3.getTransform()->rotateCenter(TransformIdentity::right(), -90.0f);
    zid3.getTransform()->scale(10);
    zid3.getTransform()->translate(glm::vec3(0, 1, 1));
    engine->AddObject(&zid3);
    Plane zid4("plane", 1, 1, glm::vec3(0.1, 0.6, 0.1));
    zid4.getTransform()->rotateCenter(TransformIdentity::up(), -90.0f);
    zid4.getTransform()->scale(10);
    zid4.getTransform()->translate(glm::vec3(-1, 0, 1));
    // engine->AddObject(&zid4);
    Plane strop("plane", 1, 1, glm::vec3(0.3, 0.1, 0.3));
    strop.getTransform()->rotateCenter(TransformIdentity::up(), 180.0f);
    strop.getTransform()->scale(10);
    strop.getTransform()->translate(glm::vec3(0, 0, 2));
    engine->AddObject(&strop);
    Plane svica("plane", 1, 1, glm::vec3(1, 1, 1));
    svica.getTransform()->rotateCenter(TransformIdentity::up(), 180.0f);
    svica.getTransform()->translate(glm::vec3(0, 0, 19.99));
    svica.getTransform()->scale(2.5);
    svica.material->colorEmissive = glm::vec3(1, 1, 1);
    svica.material->emissiveStrength = 20;
    engine->AddObject(&svica);

    // glm::vec3 u, v;
    // p.uv(u, v);
    // PolyLine l1(glm::vec3(1, 0, 0)), l2(glm::vec3(0, 0, 1));
    // l1.addPoint(p.getTransform()->apply(p.mesh->getVertex(0)));
    // l1.addPoint(p.getTransform()->apply(p.mesh->getVertex(0)) + u);
    // l2.addPoint(p.getTransform()->apply(p.mesh->getVertex(0)));
    // l2.addPoint(p.getTransform()->apply(p.mesh->getVertex(0)) + v);
    // l1.commit();
    // l2.commit();
    // engine->AddObject(&l1);
    // engine->AddObject(&l2);
    UI::AddBuilderFunction([&]() {
        ImGui::SliderInt("Depth", &Renderer::raytracer.depth, 0, 10);
        ImGui::SliderInt("Rays per pixel", &Renderer::raytracer.rpp, 0, 10);
        ImGui::Checkbox("Render spheres", &Renderer::raytracer.settings.renderSpheres);
        ImGui::Checkbox("Render planes", &Renderer::raytracer.settings.renderPlanes);
        ImGui::Checkbox("Render meshes", &Renderer::raytracer.settings.renderMeshes);
        if (ImGui::SliderInt("Grid size", &Renderer::raytracer.hardware.gridSize, 4, 32)) {
            Renderer::raytracer.hardware.gridSize = (Renderer::raytracer.hardware.gridSize / 4) * 4;
        }
        ImGui::SliderFloat("Blurriness (1.0 = AA)", &Renderer::raytracer.settings.blurriness, 0, 100);
        ImGui::SliderFloat("Dof strength", &Renderer::raytracer.settings.dofStrength, 0, 100);
        ImGui::SliderFloat("Dof distance", &Renderer::raytracer.settings.dofDistance, 0, 10);
    });

    Input::addPerFrameListener([&](auto a) {
        float deltaTime = a.deltaTime;
        float multiplier = moveSensitivity * deltaTime;

        if (Input::checkKeyEvent(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS)) {
            multiplier *= sprintMultiplier;
        }
        if (Input::checkKeyEvent(GLFW_KEY_W, GLFW_PRESS)) {
            camera->setPosition(camera->position() + multiplier * camera->forward());
            camera->recalculateMatrix();
        }
        if (Input::checkKeyEvent(GLFW_KEY_A, GLFW_PRESS)) {
            camera->translate(multiplier * -TransformIdentity::right());
            camera->recalculateMatrix();
        }
        if (Input::checkKeyEvent(GLFW_KEY_S, GLFW_PRESS)) {
            camera->translate(multiplier * -TransformIdentity::forward());
            camera->recalculateMatrix();
        }
        if (Input::checkKeyEvent(GLFW_KEY_D, GLFW_PRESS)) {
            camera->translate(multiplier * TransformIdentity::right());
            camera->recalculateMatrix();
        }
        if (Input::checkKeyEvent(GLFW_KEY_SPACE, GLFW_PRESS)) {
            camera->translate(multiplier * camera->vertical());
            camera->recalculateMatrix();
        }
        if (Input::checkKeyEvent(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS)) {
            camera->translate(-multiplier * camera->vertical());
            camera->recalculateMatrix();
        }
    });

    Input::addPerFrameListener([&](auto a) {
        (void)a;

        if (Input::checkKeyEvent(GLFW_KEY_ESCAPE, GLFW_PRESS) ||
            Input::ControllerButtonPressed(XboxOneButtons::START)) {
            engine->SetShouldClose();
        }
        if (Input::ControllerButtonPressed(XboxOneButtons::B)) {
            engine->SetGUIEnabled(!engine->guiEnabled);
        }
    });

    Input::addKeyEventListener([&](InputGlobalListenerData event) {
        if (event.action != GLFW_PRESS && event.action != GLFW_REPEAT)
            return;
        Camera *camera = engine->GetCamera();

        if (event.key == GLFW_KEY_H) {
            std::cout << "##################" << std::endl;
            std::cout << "Forward:  " << glm::to_string(camera->forward()) << std::endl;
            std::cout << "Up:     " << glm::to_string(camera->up()) << std::endl;
            std::cout << "Right:  " << glm::to_string(camera->right()) << std::endl;
            std::cout << "Position: " << glm::to_string(camera->position()) << std::endl;
            std::cout << "#################" << std::endl;
        }
        if (event.key == GLFW_KEY_1) {
            Renderer::SetRenderingMethod(RenderingMethod::Rasterize);
        }
        if (event.key == GLFW_KEY_2) {
            Renderer::SetRenderingMethod(RenderingMethod::Pathtrace);
        }
        if (event.key == GLFW_KEY_3) {
            Engine::SetActiveRendering(!Engine::activeRendering);
        }
        if (event.key == GLFW_KEY_4) {
            Renderer::raytracer.setIntegrationEnabled(!Renderer::raytracer.integrationEnabled());
        }
    });

    engine->EnableVSync();

    engine->Loop();

    return EXIT_SUCCESS;
}
