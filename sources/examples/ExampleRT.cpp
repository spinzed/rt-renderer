#include "examples/ExampleRT.h"

// Local Headers
#include "core/Animation.h"
#include "core/Animator.h"
#include "core/Behavior.h"
#include "core/Camera.h"
#include "core/Input.h"
#include "core/Transform.h"
#include "core/UI.h"
#include "core/WindowManager.h"
#include "models/Mesh.h"
#include "renderer/Renderer.h"
#include "objects/MeshObject.h"
#include "objects/PointCloud.h"
#include "renderer/Cubemap.h"
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

void ExampleRT::cursorPositionCallback(WindowCursorEvent event) {
    if (!engine->manager->focused)
        return;

    int dx = -event.xpos;
    int dy = -event.ypos;
    Camera *camera = engine->GetCamera();

    camera->rotate(mouseSensitivity * dx, mouseSensitivity * dy);
    camera->recalculateMatrix();

    // engine->manager->CenterCursor();
    // engine->manager->SetCursorPosition(dx, dy);
    engine->manager->SetCursorPosition(0, 0);
}

int ExampleRT::run(std::string execDirectory) {
    engine->Init(width, height, execDirectory);

    engine->manager->SetCursorMode(CursorMode::DISABLED);

    engine->manager->setCursorCallback([&](auto a) { cursorPositionCallback(a); });
    engine->manager->setWindowFocusCallback([&](auto data) {
        // FIXME: when clicking on window focused goes to 1, but it doesn't change the cursor pos
        if (data.focused) {
            engine->manager->CenterCursor();
        }
    });

    /*********************************************************************************************/
    Camera *camera = engine->GetCamera();
    camera->translate(glm::vec3(3.0f, 3.0f, -3.0f));
    camera->rotate(145, -30);
    camera->recalculateMatrix();

    Cubemap skybox = Cubemap::Load("skybox");
    engine->SetSkybox(&skybox);

    Mesh *mesh = Mesh::Load("kocka");
    MeshObject pod("pod", mesh);
    pod.getTransform()->scale(glm::vec3(5, 1, 5));

    engine->AddObject(&pod);

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
    });

    engine->EnableVSync();

    engine->Loop();

    return EXIT_SUCCESS;
}
