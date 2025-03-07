#include "core/UI.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

void UI::Init(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange; // don't let imgui assume control of the mouse

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    ImGui::GetStyle().ScaleAllSizes(1.5f); 
}

void UI::AddBuilderFunction(std::function<void()> f) { builderFuncs.emplace_back(f); }
void UI::Build(std::function<void()> f) { oneTimeBuilderFuncs.emplace_back(f); }

// goes in the render loop
void UI::BuildUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug");

    //float val;
    //ImGui::SliderFloat("label", &val, 2, 5, ".5f");

    for (const auto &func : oneTimeBuilderFuncs) {
        func();
    }
    oneTimeBuilderFuncs.clear();

    for (const auto &func : builderFuncs) {
        func();
    }

    ImGui::End();
}

void UI::Render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::Cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
