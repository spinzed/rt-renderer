#include "core/Engine.h"

#include "core/Animator.h"
#include "core/Input.h"
#include "core/UI.h"
#include "core/WindowManager.h"
#include "renderer/Renderer.h"

void Engine::Init(int width, int height, std::string execDirectory) {
    manager = new WindowManager(width, height, 60, 1.0, "Engine");

    // init subsystems
    Shader::setBaseDirectory(execDirectory + "/shaders");
    Loader::setPath(execDirectory + "/resources");
    Renderer::Init(width, height);
    UI::Init(manager->window);

    // input system settings
    Input::init(manager->window);
    Input::boundsGetter = [&](int w, int h) { manager->GetBounds(w, h); };
    Input::addKeyEventListener([&](InputGlobalListenerData data) {
        if (data.action == GLFW_PRESS && data.key == GLFW_KEY_RIGHT_SHIFT) {
            SetGUIEnabled(!guiEnabled);
        }
        if (data.action == GLFW_PRESS && data.key == GLFW_KEY_RIGHT_CONTROL) {
            manager->SetIgnoreMouseEvents(!manager->mouseEventsIgnored);
            manager->SetCursorMode(manager->CursorMode() == CursorMode::NORMAL ? CursorMode::DISABLED
                                                                               : CursorMode::NORMAL);
        }
    });
    Input::addPerFrameListener([&](auto _) {
        (void)_;
        if (Input::ControllerButtonPressed(XboxOneButtons::L3)) {
            SetGUIEnabled(!guiEnabled);
        }
    });

    // window manager settings
    SetResolution(width, height);
    manager->SetResizeCallback([&](int width, int height) { SetResolution(width, height); });

    // setup camera
    camera = std::make_unique<Camera>(width, height);
    camera->addChangeListener([] { Renderer::_cameraMatrixChanged = true; });
}

void Engine::Loop() {
    while (!manager->WantsToClose()) {
        float deltaTime = (float)manager->LimitFPS(false);
        debugString = "Engine Debug String\n";
        Timer t = Timer::start();
        debugString += "Profiling started\n";

        Input::ClearControllerStates();
        // ask undelying window manager to poll all queued events
        manager->PollEvents();

        // fire the subsystems
        Input::firePerFrame(deltaTime);
        Animator::passTime(deltaTime);
        ParticleSystem::passTime(deltaTime);
        debugString += t.format("Subsystems Fired: # ($)\n");

        // run game logic - update the object every tick according to the custom behavior scripts
        for (Object *o : objects) {
            for (Behavior *behavior : o->behaviors) {
                if (!behavior->initialized) {
                    behavior->Init(o);
                    behavior->initialized = true;
                }
                behavior->Update(o, deltaTime);
            }
            for (Object *child : o->children) {
                for (Behavior *behavior : child->behaviors) {
                    if (!behavior->initialized) {
                        behavior->Init(child);
                        behavior->initialized = true;
                    }
                    behavior->Update(child, deltaTime);
                }
            }
        }
        debugString += t.format("Game logic ran: # ($)\n");

        // if not active rendering, wait a bit to prevent cpu choke and render the next frame
        if (!activeRendering) {
            std::this_thread::sleep_for(std::chrono::microseconds(16667));
            continue;
        }

        Clear();
        Render();

        SwapBuffers();

        UI::Build([&]() { ImGui::Text(debugString.c_str()); });
        UI::Build([&]() { ImGui::Text(Renderer::debugString.c_str()); });
        std::string finalTime = t.format("Rendered and swapped: # ($)\n");
        UI::Build([&]() { ImGui::Text(finalTime.c_str()); });
    }

    manager->Destroy();
}

void Engine::Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void Engine::SetShouldClose() { glfwSetWindowShouldClose(manager->window, true); }

void Engine::SetResolution(int width, int height) {
    Renderer::SetResolution(width, height);
    if (camera != nullptr) {
        camera->setSize(width, height);
    }
}

Camera *Engine::GetCamera() { return camera.get(); }

void Engine::AddObject(Object *o) { objects.push_back(o); }
void Engine::RemoveObject(Object *o) { objects.erase(std::remove(objects.begin(), objects.end(), o), objects.end()); }

void Engine::AddLight(Light *l) { lights.push_back(l); }

void Engine::AddParticleCluster(ParticleCluster *pc) { ParticleSystem::registerCluster(pc); }

void Engine::Render() {
    if (guiEnabled) {
        UI::BuildUI();
    }

    RenderData data = {
        .objects = &objects,
        .lights = &lights,
        .skybox = skybox,
        .camera = GetCamera(),
    };

    Renderer::Render(data);

    if (guiEnabled) {
        UI::Render();
    }
}

void Engine::SetGUIEnabled(bool e) {
    guiEnabled = e;
    // if (e) {
    //     savedCursorMode = manager->CursorMode();
    //     manager->SetCursorMode(CursorMode::NORMAL);
    // } else {
    //     manager->SetCursorMode(savedCursorMode);
    // }
    // manager->SetIgnoreMouseEvents(e);
}

void Engine::SetActiveRendering(bool b) { activeRendering = b; }

void Engine::EnableVSync() {
    glfwSwapInterval(1);
    vsync = 1;
}

void Engine::DisableVSync() {
    glfwSwapInterval(0);
    vsync = 0;
}

void Engine::SwapBuffers() { glfwSwapBuffers(manager->window); }
