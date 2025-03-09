#include "utils/Timer.h"

#include <chrono>
#include <iostream>
#include <string>
#include <cmath>

Timer::Timer() { reset(); }

Timer Timer::start() { return Timer(); }

std::string _format(double elapsedTime) {
    return std::format("{:.2f}", elapsedTime) + " ms";
}

void Timer::reset() {
    // Capture the current time point using high-resolution clock
    t1 = tlast = std::chrono::high_resolution_clock::now();
}

double Timer::elapsed() {
    // Get the current time point and compute the difference in milliseconds
    tlast = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsedTime = tlast - t1;
    return elapsedTime.count();
}
std::string Timer::elapsedFormatted() {
    return _format(elapsed());
}

double Timer::checkpoint() {
    ttemp = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsedTime = ttemp - tlast;
    tlast = ttemp;
    return elapsedTime.count();
}

std::string Timer::checkpointFormatted() {
    return _format(checkpoint());
}

std::string Timer::format(std::string message) {
    size_t indexDollar = message.find("$");
    size_t indexPound = message.find("#");
    if (indexDollar == std::string::npos && indexPound == std::string::npos) {
        double elapsedTime = elapsed();
        message += _format(elapsedTime);
    } else {
        if (indexPound != std::string::npos) {
            double elapsedTime = checkpoint();
            message.replace(indexPound, 1, _format(elapsedTime));
        }
        indexDollar = message.find("$");
        if (indexDollar != std::string::npos) {
            double elapsedTime = elapsed();
            message.replace(indexDollar, 1, _format(elapsedTime));
        }
    }
    return message;
}

void Timer::printFormatted(std::string message) {
    std::cout << format(message) << std::endl;
}
