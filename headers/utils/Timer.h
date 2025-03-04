#pragma once

#include <chrono>
#include <string>

class Timer {
  public:
    static Timer start();
    void reset();
    double elapsed();
    std::string elapsedFormatted();
    double checkpoint();
    std::string checkpointFormatted();
    std::string format(std::string message);
    void printFormatted(std::string message);

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> t1, tlast, ttemp;
    Timer();
};
