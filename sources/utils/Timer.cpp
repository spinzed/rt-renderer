#include "utils/Timer.h"
#include <chrono>
#include <iostream>
#include <string>

Timer::Timer() { reset(); }

Timer Timer::start() { return Timer(); }

void Timer::reset() {
    // Capture the current time point using high-resolution clock
    t1 = std::chrono::high_resolution_clock::now();
}

double Timer::elapsed() {
    // Get the current time point and compute the difference in milliseconds
    tlast = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsedTime = tlast - t1;
    return elapsedTime.count();
}

double Timer::checkpoint() {
    ttemp = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsedTime = ttemp - tlast;
    tlast = ttemp;
    return elapsedTime.count();
}

void Timer::printElapsed(std::string message) {
    size_t indexDollar = message.find("$");
    size_t indexPound = message.find("#");
    if (indexDollar == std::string::npos && indexPound == std::string::npos) {
        double elapsedTime = elapsed();
        message += std::to_string(elapsedTime) + "ms";
    } else {
        if (indexDollar == std::string::npos) {

            double elapsedTime = elapsed();
            message.replace(indexDollar, 1, std::to_string(elapsedTime) + "ms");
        }
        if (indexPound == std::string::npos) {
            double elapsedTime = elapsed();
            message.replace(indexPound, 1, std::to_string(elapsedTime) + "ms");
        }
    }
    std::cout << message << std::endl;
}
