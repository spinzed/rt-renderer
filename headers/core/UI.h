#pragma once

#include <GLFW/glfw3.h>

#include <vector>
#include <functional>

class UI {
  public:
    inline static std::vector<std::function<void()>> builderFuncs;
    inline static std::vector<std::function<void()>> oneTimeBuilderFuncs;

    static void Init(GLFWwindow *window);
    static void AddBuilderFunction(std::function<void()> f); // persistent builder function
    static void Build(std::function<void()> f); // one-time builder function
    static void BuildUI();
    static void Render();
    static void Cleanup();
};
