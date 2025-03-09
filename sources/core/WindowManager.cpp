#include "core/WindowManager.h"

#include "GLFW/glfw3.h"
#include "utils/GLDebug.h"

#include <iostream>
#include <sstream>
#include <thread>

const double WindowManager::MIN_TARGET_FPS = 5.0;
const double WindowManager::MAX_TARGET_FPS = 1000.0;

void WindowManager::init(int width, int height, double theTargetFps, bool theVerboseSetting) {
    setupOpenGL(width, height);

    setTargetFps(theTargetFps);

    totalFrameCount = 0;
    frameCount = 0;
    currentFps = 0.0;
    sleepDuration = 0.0;
    frameStartTime = glfwGetTime();
    frameEndTime = frameStartTime + 1;
    frameDuration = 1;
    lastReportTime = frameStartTime;
    reportInterval = 1.0f;
    windowTitle = "NONE";
    verbose = theVerboseSetting;
    this->width = width;
    this->height = height;
    SetCursorMode(CursorMode::NORMAL);

    glfwSetWindowUserPointer(window, static_cast<void *>(this)); // TODO: get rid of this
    glfwSetFramebufferSizeCallback(window, resizeCallbackWrapper);
    glfwSetWindowFocusCallback(window, windowFocusCallbackWrapper);
    glfwSetCursorPosCallback(window, windowCursorCallbackWrapper);
}

void WindowManager::setupOpenGL(int width, int height) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    window = glfwCreateWindow(width, height, "Renderer", nullptr, nullptr);
    if (window == nullptr) {
        fprintf(stderr, "Failed to Create OpenGL Context\n");
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();

    // dohvati sve dostupne OpenGL funkcije
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        exit(-1);
    }
    fprintf(stdout, "OpenGL %s\n", glGetString(GL_VERSION));
    GLCheckError();
}

WindowManager::WindowManager(int width, int height, int theTargetFps) { init(width, height, theTargetFps, false); }

WindowManager::WindowManager(int width, int height, int theTargetFps, double theReportInterval) {
    init(width, height, theTargetFps, true);
    setReportInterval(theReportInterval);
}

WindowManager::WindowManager(int width, int height, int theTargetFps, float theReportInterval,
                             std::string theWindowTitle) {
    init(width, height, theTargetFps,
         true); // If you specify a window title it's safe to say you want the FPS to update there ;)
    setReportInterval(theReportInterval);
    windowTitle = theWindowTitle;
}

bool WindowManager::getVerbose() { return verbose; }

void WindowManager::setVerbose(bool theVerboseValue) { verbose = theVerboseValue; }

int WindowManager::getTargetFps() { return targetFps; }

void WindowManager::setTargetFps(int theFpsLimit) {
    // Make at least some attempt to sanitise the target FPS...
    if (theFpsLimit < MIN_TARGET_FPS) {
        theFpsLimit = MIN_TARGET_FPS;
        std::cout << "Limiting FPS rate to legal minimum of " << MIN_TARGET_FPS << " frames per second." << std::endl;
    }
    if (theFpsLimit > MAX_TARGET_FPS) {
        theFpsLimit = MAX_TARGET_FPS;
        std::cout << "Limiting FPS rate to legal maximum of " << MAX_TARGET_FPS << " frames per second." << std::endl;
    }

    // ...then set it and calculate the target duration of each frame at this framerate
    targetFps = theFpsLimit;
    targetFrameDuration = 1.0 / targetFps;
}

double WindowManager::getFrameDuration() { return frameDuration; }

void WindowManager::setReportInterval(float theReportInterval) {
    // Ensure the time interval between FPS checks is sane (low cap = 0.1s, high-cap = 10.0s)
    // Negative numbers are invalid, 10 fps checks per second at most, 1 every 10 secs at least.
    if (theReportInterval < 0.1) {
        theReportInterval = 0.1;
    }
    if (theReportInterval > 10.0) {
        theReportInterval = 10.0;
    }
    reportInterval = theReportInterval;
}

double WindowManager::LimitFPS(bool shouldSleep) {
    totalFrameCount++;

    frameEndTime = glfwGetTime();

    frameDuration = frameEndTime - frameStartTime;

    if (reportInterval != 0.0f) {
        if ((frameEndTime - lastReportTime) > reportInterval) {
            lastReportTime = frameEndTime;

            currentFps = (double)frameCount / reportInterval;

            frameCount = 1;

            if (verbose) {
                // std::cout << "FPS: " << currentFps << std::endl;

                if (windowTitle != "NONE") {
                    std::ostringstream stream;
                    stream << currentFps;
                    std::string fpsString = stream.str();

                    std::string tempWindowTitle = windowTitle + " | FPS: " + fpsString;

                    const char *pszConstString = tempWindowTitle.c_str();
                    glfwSetWindowTitle(window, pszConstString);
                }
            }

        } else {
            ++frameCount;
        }
    }

    sleepDuration = targetFrameDuration - frameDuration;

    if (shouldSleep && sleepDuration > 0.0) {
        std::this_thread::sleep_for(std::chrono::microseconds((int)(1000000 * (targetFrameDuration - frameDuration))));
    }

    frameStartTime = glfwGetTime();

    return frameDuration + (frameStartTime - frameEndTime);
}

int WindowManager::getFrameCount() { return totalFrameCount; }

void WindowManager::PollEvents() { glfwPollEvents(); }

bool WindowManager::WantsToClose() {
    return glfwWindowShouldClose(window);
}

void WindowManager::Destroy() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
