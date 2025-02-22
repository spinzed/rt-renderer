#pragma once

// #include "Helper.h"
#ifndef __glfw_h_
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#endif

#include <functional>
#include <iostream>
#include <string>

struct WindowCursorEvent {
    double xpos;
    double ypos;
};

struct WindowFocusEvent {
    bool focused;
};

enum CursorMode {
    NORMAL,
    HIDDEN,
    DISABLED,
};

class WindowManager {
  private:
    double frameStartTime; // Frame start time
    double frameEndTime;   // Frame end time
    double frameDuration;  // How many milliseconds between the last frame and this frame

    double targetFps;    // The desired FPS to run at (i.e. maxFPS)
    double currentFps;   // The current FPS value
    int frameCount;      // How many frames have been drawn since last update
    int totalFrameCount; // How many frames have been drawn since the program start

    double targetFrameDuration; // How many milliseconds each frame should take to hit a target FPS value (i.e. 60fps
                                // = 1.0 / 60 = 0.016ms)
    double sleepDuration;       // How long to sleep if we're exceeding the target frame rate duration

    double lastReportTime; // The timestamp of when we last reported
    double reportInterval; // How often to update the FPS value

    std::string windowTitle; // Window title to update view GLFW

    bool verbose; // Whether or not to output FPS details to the console or update the window

    // Limit the minimum and maximum target FPS value to relatively sane values
    static const double MIN_TARGET_FPS;
    static const double
        MAX_TARGET_FPS; // If you set this above the refresh of your monitor and enable VSync it'll break! Be aware!

    // Private method to set relatively sane defaults. Called by constructors before overwriting with more specific
    // values as required.
    void init(int weight, int height, double theTargetFps, bool theVerboseSetting);

    void setupOpenGL(int weight, int height);

    std::function<void(int, int)> resizeCallback;
    static void resizeCallbackWrapper(GLFWwindow *window, int width, int height) {
        WindowManager *manager = static_cast<WindowManager *>(glfwGetWindowUserPointer(window));
        manager->width = width;
        manager->height = height;
        if (manager && manager->resizeCallback) {
            manager->resizeCallback(width, height);
        }
    }

    inline static std::function<void(WindowCursorEvent)> windowCursorCallback;
    static void windowCursorCallbackWrapper(GLFWwindow *window, double xpos, double ypos) {
        (void)window;
        if (mouseEventsIgnored)
            return;

        if (windowCursorCallback) {
            WindowCursorEvent event = {.xpos = xpos, .ypos = ypos};
            windowCursorCallback(event);
        }
    }

    inline static std::function<void(WindowFocusEvent)> windowFocusCallback;
    static void windowFocusCallbackWrapper(GLFWwindow *window, int f) {
        (void)window;
        focused = f;

        if (windowFocusCallback) {
            WindowFocusEvent event = {.focused = focused};
            windowFocusCallback(event);
        }
    }

  public:
    GLFWwindow *window;
    int width;
    int height;
    inline static bool focused = 1; // on windows, it is by default 0 and it doesn't update unless alt tab

    // Single parameter constructor - just set a desired framerate and let it go.
    // Note: No FPS reporting by default, although you can turn it on or off later with the setVerbose(true/false)
    // method
    WindowManager(int width, int height, int theTargetFps);

    // Two parameter constructor which sets a desired framerate and a reporting interval in seconds
    WindowManager(int width, int height, int theTargetFps, double theReportInterval);

    // Three parameter constructor which sets a desired framerate, how often to report, and the window title to
    // append the FPS to
    WindowManager(int width, int height, int theTargetFps, float theReportInterval, std::string theWindowTitle);

    // Getter and setter for the verbose property
    bool getVerbose();
    void setVerbose(bool theVerboseValue);

    // Getter and setter for the targetFps property
    int getTargetFps();
    void setTargetFps(int theFpsLimit);

    // Returns the time it took to complete the last frame in milliseconds
    double getFrameDuration();

    // Setter for the report interval (how often the FPS is reported) - santises input.
    void setReportInterval(float theReportInterval);

    // Method to force our application to stick to a given frame rate and return how long it took to process a frame
    double LimitFPS(bool shouldSleep);

    int getFrameCount();

    inline static bool mouseEventsIgnored = false;
    void PollEvents();
    void SetIgnoreMouseEvents(bool e) { mouseEventsIgnored = e; }

    void GetCursorPosition(double *x, double *y) { glfwGetCursorPos(window, x, y); }

    void SetCursorPosition(float width, float height) { glfwSetCursorPos(window, width, height); }

    void CenterCursor() { SetCursorPosition((float)width / 2, (float)height / 2); }

    void SetCursorMode(CursorMode mode) {
        switch (mode) {
        case CursorMode::NORMAL:
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        case CursorMode::HIDDEN:
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            break;
        case CursorMode::DISABLED:
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
        default:
            std::cout << "unsuppored cursor mode" << std::endl;
            break;
        }
    }

    CursorMode CursorMode() {
        switch (glfwGetInputMode(window, GLFW_CURSOR)) {
        case GLFW_CURSOR_NORMAL:
            return CursorMode::NORMAL;
        case GLFW_CURSOR_HIDDEN:
            return CursorMode::HIDDEN;
        case GLFW_CURSOR_DISABLED:
            return CursorMode::DISABLED;
        default:
            std::cout << "unsuppored glfw cursor mode" << std::endl;
            return CursorMode::NORMAL;
        }
    }

    void SetResizeCallback(std::function<void(int, int)> l) { resizeCallback = l; }

    void setCursorCallback(std::function<void(WindowCursorEvent)> l) { windowCursorCallback = l; }

    void setWindowFocusCallback(std::function<void(WindowFocusEvent)> l) { windowFocusCallback = l; }

    void GetBounds(int &width, int &height) {
        width = this->width;
        height = this->height;
    }

    bool WantsToClose();

    void Destroy();
};