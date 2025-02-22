#include "examples/ExampleGame.h"

#include "core/Animation.h"
#include "core/Animator.h"

// Local Headers
#include "models/Mesh.h"
#include "objects/MeshObject.h"
#include "objects/PointCloud.h"
#include "core/Behavior.h"
#include "core/Camera.h"
#include "renderer/Cubemap.h"
#include "core/Input.h"
#include "renderer/Shader.h"
#include "core/Transform.h"
#include "core/UI.h"
#include "core/WindowManager.h"
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

void ExampleGame::KeyCallback(InputGlobalListenerData event) {
    if (event.action != GLFW_PRESS && event.action != GLFW_REPEAT)
        return;
    Camera *camera = app->GetCamera();

    if (event.key == GLFW_KEY_H) {
        std::cout << "##################" << std::endl;
        std::cout << "Forward:  " << glm::to_string(camera->forward()) << std::endl;
        std::cout << "Up:     " << glm::to_string(camera->up()) << std::endl;
        std::cout << "Right:  " << glm::to_string(camera->right()) << std::endl;
        std::cout << "Position: " << glm::to_string(camera->position()) << std::endl;
        std::cout << "#################" << std::endl;
    }
}

void ExampleGame::cursorPositionCallback(WindowCursorEvent event) {
    if (!app->manager->focused)
        return;

    int dx = (float)app->manager->width / 2 - event.xpos;
    int dy = (float)app->manager->height / 2 - event.ypos;
    Camera *camera = app->GetCamera();

    camera->rotate(mouseSensitivity * dx, mouseSensitivity * dy);
    camera->recalculateMatrix();

    app->manager->CenterCursor();
}

int ExampleGame::run(std::string execDirectory) {
    app->Init(width, height, execDirectory);

    // renderer->input.addMouseListener([](MouseClickOptions _) {});
    Input::addKeyEventListener([&](auto a) { KeyCallback(a); });
    app->manager->SetCursorMode(CursorMode::DISABLED);

    app->manager->setCursorCallback([&](auto a) { cursorPositionCallback(a); });
    app->manager->setWindowFocusCallback([&](auto data) {
        // FIXME: when clicking on window focused goes to 1, but it doesn't change the cursor pos
        if (data.focused) {
            app->manager->CenterCursor();
        }
    });

    /*********************************************************************************************/
    Shader *fullbrightShader = Shader::Load("fullbright");

    ThreadPool pool(5);

    Camera *camera = app->GetCamera();
    camera->translate(glm::vec3(3.0f, 3.0f, -3.0f));
    camera->rotate(145, -30);
    camera->recalculateMatrix();

    // Cubemap skybox = Cubemap::Load("skybox");
    Cubemap skybox = Cubemap::Load({
        "kurt/space_rt.png",
        "kurt/space_lf.png",
        "kurt/space_up.png",
        "kurt/space_dn.png",
        "kurt/space_ft.png",
        "kurt/space_bk.png",
    });
    app->SetSkybox(&skybox);

    float size = 25;
    float currentFirstPolje = 0;
    float currentPoljeOffset = 0; // changes to 1 afterFirst

    int segments = 10;

    std::vector<PointCloud *> polja(segments);
    std::vector<Field> ponasanja(segments);

    for (int i = 0; i < segments; i++) {
        polja[i] = new PointCloud();
        polja[i]->addBehavior(&ponasanja[i]);
        // ponasanja[i].generate(i * size, (i + 1) * size);
        app->AddObject(polja[i]);
    }

    // player
    Mesh *arwingMesh = Mesh::Load("arwing");
    MeshObject arwing("playerModel", arwingMesh, fullbrightShader);
    arwing.getTransform()->rotate(TransformIdentity::up(), 90.0f);

    Object player("player");
    player.addChild(&arwing);

    FunctionalBehavior playerBehavior;

    glm::vec3 initialSpeed(12.0f, 0.0f, 0.0f);
    glm::vec3 maxSpeed(999, 8.0f, 12.0f); // goes in both directions
    glm::vec3 speed(initialSpeed);
    glm::vec3 boundsMin(-INFINITY, -1, 0);
    glm::vec3 boundsMax(INFINITY, 5, size);
    // 1 point every 6 secs, get to max speed in 250 ms
    glm::vec3 accel(1.0f / 6, maxSpeed.y / 0.250f, maxSpeed.z / 0.250f);
    glm::vec3 initialCameraOffset(20, 1.5, 1);
    glm::vec3 currentCameraOffset;
    glm::vec3 initialPlayerPosition(-20.0f, (boundsMax.y + boundsMin.y) * 0.3f, 12.5f);
    glm::vec3 firstPlayerPosition(-30.0f, (boundsMax.y + boundsMin.y) * 0.3f, 12.5f);

    Transform initialPlayerTransform;
    initialPlayerTransform.translate(initialPlayerPosition);
    initialPlayerTransform.scale(0.2f);
    player.getTransform()->copyFrom(initialPlayerTransform);

    bool invertZ = false;
    bool cameraOffsetSet = false;
    bool gamePaused = false;
    bool gameOver = true;
    bool collisionsActive = true;
    bool freeCamera = false;
    bool cameraStatic = true;

    auto setFieldSettings = [&](Field *f, int start) {
        int segment = start / size;
        if (segment < 8) {
            f->suppressGeneration = false;
            f->threshold = 0.5;
            f->stripEdgeMask = 0b0000 | (segment == 0 ? 0b1000 : 0) | (segment == 7 ? 0b0100 : 0);
        } else if (segment == 8) {
            f->suppressGeneration = true;
        } else if (segment < 16) {
            f->suppressGeneration = false;
            f->threshold = 0.3;
            f->stripEdgeMask = 0b0000 | (segment == 9 ? 0b1000 : 0) | (segment == 15 ? 0b0100 : 0);
        } else if (segment == 16) {
            f->suppressGeneration = true;
        } else {
            f->suppressGeneration = false;
            f->threshold = 0.25;
            f->stripEdgeMask = 0b0000 | (segment == 17 ? 0b1000 : 0);
        }
    };

    auto generateField = [&](int i, int start, bool goNext) {
        Field *b = (Field *)polja[i]->behaviors[0];
        b->generateMutex.lock();
        b->busy = true;

        if (goNext && currentPoljeOffset != 0) {
            auto firstElement = polja.front();
            polja.erase(polja.begin());
            polja.emplace_back(firstElement);
        }

        if (goNext) {
            if (currentPoljeOffset == 0) {
                currentPoljeOffset = 1;
            } else {
                currentFirstPolje++;
            }
        }
        setFieldSettings(b, start);
        b->generate(start, start + size);

        b->busy = false;
        b->generateMutex.unlock();
    };

    auto setGamePaused = [&](bool paused) { gamePaused = paused; };

    TransformAnimation animScale = TransformAnimation::scale(1.5, player.getTransform(), glm::vec3(0));
    TransformAnimation animRotate =
        TransformAnimation::rotate(1.5, player.getTransform(), TransformIdentity::up(), 3 * 360.0f);

    auto stopGame = [&]() {
        gameOver = true;
        float score = std::round(player.getTransform()->position().x * 100) / 100;
        Animator::registerAnimation(&animScale);
        Animator::registerAnimation(&animRotate);
        std::cout << "Score: " << score << std::endl;
    };

    auto resetGame = [&]() {
        gamePaused = false;
        gameOver = false;

        Animator::unregisterAnimation(&animScale);
        Animator::unregisterAnimation(&animRotate);
        perlin.randomizeSeed();
        player.getTransform()->copyFrom(initialPlayerTransform);
        speed = initialSpeed;
        currentPoljeOffset = 0;
        currentFirstPolje = 0;

        for (Field &b : ponasanja) {
            b.generateMutex.lock();
            b.reset();
            b.generateMutex.unlock();
        }

        for (int i = 0; i < segments; i++) {
            int j = i;
            pool.enqueue([&, j]() { generateField(j, j * size, false); });
        }
    };

    playerBehavior.onInit = [&](Object *_) {
        (void)_;
        resetGame();
        currentCameraOffset = initialCameraOffset;
        player.getTransform()->setPosition(firstPlayerPosition);
        camera->setPosition(player.getTransform()->position() + initialCameraOffset);
    };

    playerBehavior.onUpdate = [&](Object *player, float deltaTime) {
        glm::vec3 pos = player->getTransform()->position();

        UI::Build([&speed, pos, &collisionsActive, &freeCamera]() {
            bool c = Input::ControllerConnected();
            ImGui::Text("Controller connected: %s", c ? Input::GetControllerName() : "No");
            ImGui::Text("Pos: %s", glm::to_string(pos).c_str());
            ImGui::Text("Saved Speed: %s", glm::to_string(speed).c_str());
            if (ImGui::Button(std::format("Toggle Draw Culled ({})", showCulled ? "on" : "off").c_str())) {
                showCulled = !showCulled;
            }
            if (ImGui::Button(std::format("Toggle Collisions ({})", collisionsActive ? "on" : "off").c_str())) {
                collisionsActive = !collisionsActive;
            }
            if (ImGui::Button(std::format("Free Camera ({})", freeCamera ? "on" : "off").c_str())) {
                freeCamera = !freeCamera;
            }
            float sx = speed.x;
            if (ImGui::SliderFloat("SpeedX", &sx, 0, 100)) {
                speed.x = sx;
            }
        });

        if (gamePaused || gameOver)
            return;

        // switch fields
        Field *b = (Field *)polja[segments - 1]->behaviors[0];
        if (player->getTransform()->position().x > (currentFirstPolje + currentPoljeOffset + 1) * size && !b->busy) {
            pool.enqueue([&]() { generateField(0, (currentFirstPolje + segments) * size, 1); });
        }

        glm::vec3 dir(0);
        glm::vec3 isAutoDeccel(0);

        // keyboard
        if (Input::checkKeyEvent(GLFW_KEY_UP, GLFW_PRESS)) {
            dir.y = (invertZ ? 1 : -1) * 1;
        }
        if (Input::checkKeyEvent(GLFW_KEY_DOWN, GLFW_PRESS)) {
            dir.y = (invertZ ? 1 : -1) * -1;
        }
        if (Input::checkKeyEvent(GLFW_KEY_LEFT, GLFW_PRESS)) {
            dir.z = -1;
        }
        if (Input::checkKeyEvent(GLFW_KEY_RIGHT, GLFW_PRESS)) {
            dir.z = 1;
        }

        // decelerate if button not pressed
        if (dir.x == 0 && speed.x != 0) {
            dir.x = -glm::sign(speed.x);
            isAutoDeccel.x = 1;
        }
        if (dir.y == 0 && speed.y != 0) {
            dir.y = -glm::sign(speed.y);
            isAutoDeccel.y = 1;
        }
        if (dir.z == 0 && speed.z != 0) {
            dir.z = -glm::sign(speed.z);
            isAutoDeccel.z = 1;
        }

        // instead of x speed being controlled by keyboard, it starts at a constant and constantly increases
        // speed.x += dir.x * accel * deltaTime;
        speed.x += accel.x * deltaTime;

        // if controller is connected, calculate the speed using the analog stick
        if (Input::ControllerConnected()) {
            float valY = mtr::deadzone(Input::GetAnalog(XboxOneAnalog::LEFT_Y), 0.3f, 0, 1);
            float valZ = (invertZ ? -1 : +1) * mtr::deadzone(Input::GetAnalog(XboxOneAnalog::LEFT_X), 0.1f, 0, 1);
            speed.y = valY * maxSpeed.y;
            speed.z = valZ * maxSpeed.z;
        } else {
            speed.y += dir.y * accel.y * deltaTime;
            speed.z += dir.z * accel.z * deltaTime;
        }

        // 0 clamp
        if (isAutoDeccel.x && speed.x * dir.x > 0) {
            speed.x = 0;
        }
        if (isAutoDeccel.y && speed.y * dir.y > 0) {
            speed.y = 0;
        }
        if (isAutoDeccel.z && speed.z * dir.z > 0) {
            speed.z = 0;
        }

        // extremes clamp
        speed = glm::clamp(speed, -maxSpeed, maxSpeed);

        // apply final temporary (non linear) transformations to speed
        // make the y & z speed move on a sphere rather than a line. This will effectively
        // make the aircraft change its direction uniformally rather than doing it faster
        // when the speed is small rather than large.
        glm::vec3 transformedSpeed(                                                 //
            speed.x,                                                                //
            maxSpeed.y * glm::sin((speed.y / maxSpeed.y) * (glm::pi<float>() / 2)), //
            maxSpeed.z * glm::sin((speed.z / maxSpeed.z) * (glm::pi<float>() / 2))  //
        );

        // add some speed forward if the boost button is pressed
        if (Input::checkKeyEvent(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS) || Input::GetAnalog(XboxOneAnalog::RT) > 0) {
            transformedSpeed.x += initialSpeed.x * 0.5f;
        }
        //... or remove if slow down button is pressed
        if (Input::checkKeyEvent(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS) || Input::GetAnalog(XboxOneAnalog::LT) > 0) {
            transformedSpeed.x -= initialSpeed.x * 0.5f;
        }

        glm::vec3 oldPlayerPos = player->getTransform()->position();
        glm::vec3 newPlayerPos = oldPlayerPos + transformedSpeed * deltaTime;

        // position clamp
        newPlayerPos = glm::clamp(newPlayerPos, boundsMin, boundsMax);

        // check for collisions with the perlin walls
        glm::vec2 pointPlayer(newPlayerPos.x, newPlayerPos.z);
        glm::vec2 playerSize(0.1);

        // if collision, game end
        if (collisionsActive) {
            Field *b = (Field *)polja[currentPoljeOffset]->behaviors[0];
            if (b->checkCollisions(pointPlayer, playerSize)) {
                stopGame();
                return;
            }
        }

        // set new pos
        player->getTransform()->setPosition(newPlayerPos);

        // snap the inner model
        if (transformedSpeed != glm::vec3(0)) {
            Object *model = player->GetChild("playerModel");
            assert(model != nullptr);
            model->getTransform()->pointAtDirection(-transformedSpeed, TransformIdentity::up());
            model->getTransform()->rotate(glm::vec3(0, 0, 1), 20 * (transformedSpeed.z / maxSpeed.z));
        }

        // snap camera - calculate interpolated speed between current position of the camera the position
        // of where the camera is supposed to be in
        glm::vec3 newCameraOffset =
            glm::vec3(-3, 1.5, 0) - //
            glm::vec3((transformedSpeed.x - speed.x) / speed.x, speed.y / maxSpeed.y, speed.z / maxSpeed.z);

        glm::vec3 interpolatedCameraOffset;
        if (!cameraOffsetSet) {
            interpolatedCameraOffset = newCameraOffset;
            cameraOffsetSet = true;
        } else {
            interpolatedCameraOffset = glm::mix(currentCameraOffset, newCameraOffset, 0.95f * deltaTime * 5);
        }

        // don't snap the camera if freely controlled
        if (freeCamera)
            return;

        Transform *t = app->GetCamera();

        // don't immediately tp if the initial sequence hasn't completed
        if (!cameraStatic || currentCameraOffset.x < newCameraOffset.x) {
            cameraStatic = false;
            t->setPosition(player->getTransform()->position() + interpolatedCameraOffset);
        }
        currentCameraOffset = t->position() - player->getTransform()->position();
        t->pointAt(player->getTransform()->position(), TransformIdentity::up());
    };
    player.addBehavior(&playerBehavior);
    app->AddObject(&player);

    Input::addPerFrameListener([&](auto a) {
        if (!freeCamera)
            return;

        float deltaTime = a.deltaTime;
        float multiplier = moveSensitivity * deltaTime;

        if (Input::checkKeyEvent(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS)) {
            multiplier *= sprintMultiplier;
        }
        if (Input::checkKeyEvent(GLFW_KEY_W, GLFW_PRESS)) {
            // camera->translate(multiplier * Transform::Identity().forward());
            camera->setPosition(camera->position() + multiplier * camera->forward());
            camera->recalculateMatrix();
            // camera->translate(multiplier * camera->forward());
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
            app->SetShouldClose();
        }
        if (Input::ControllerButtonPressed(XboxOneButtons::Y)) {
            resetGame();
        }
        if (Input::ControllerButtonPressed(XboxOneButtons::PAUSE)) {
            setGamePaused(!gamePaused);
        }
        if (Input::ControllerButtonPressed(XboxOneButtons::B)) {
            app->SetGUIEnabled(!app->guiEnabled);
        }
    });

    Input::addKeyEventListener([&](InputGlobalListenerData event) {
        if (event.action != GLFW_PRESS)
            return;
        if (event.key == GLFW_KEY_R) {
            resetGame();
        }
        if (event.key == GLFW_KEY_P) {
            setGamePaused(!gamePaused);
        }
        if (event.key == GLFW_KEY_1) {
            showCulled = true;
        }
    });

    app->EnableVSync();

    app->Loop();

    return EXIT_SUCCESS;
}
