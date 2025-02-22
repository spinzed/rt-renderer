#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <random>
#include <vector>

class PerlinNoise {
  private:
    std::vector<int> permutation;

    inline static unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

  public:
    PerlinNoise() { init(); }

    void init() {
        permutation.resize(256);
        std::iota(permutation.begin(), permutation.end(), 0);
        std::shuffle(permutation.begin(), permutation.end(), std::default_random_engine(seed));
        permutation.insert(permutation.end(), permutation.begin(), permutation.end());
    }

    // Gradient function
    float gradient(int hash, float x, float y) {
        int h = hash & 3; // Convert low 2 bits of hash code
        float u = h < 2 ? x : y;
        float v = h < 2 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    // Fade function
    float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

    // Linear interpolation
    float lerp(float a, float b, float t) { return a + t * (b - a); }

    float noise(float x, float y) {
        // Find unit grid cell containing point
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        // Relative coordinates in cell
        x -= std::floor(x);
        y -= std::floor(y);

        // Fade curves for x and y
        float u = fade(x);
        float v = fade(y);

        // Hash coordinates of the corners
        int aa = permutation[X] + Y;
        int ab = permutation[X] + Y + 1;
        int ba = permutation[X + 1] + Y;
        int bb = permutation[X + 1] + Y + 1;

        // Add blended results from corners
        float grad_aa = gradient(permutation[aa], x, y);
        float grad_ba = gradient(permutation[ba], x - 1, y);
        float grad_ab = gradient(permutation[ab], x, y - 1);
        float grad_bb = gradient(permutation[bb], x - 1, y - 1);

        float lerp_x1 = lerp(grad_aa, grad_ba, u);
        float lerp_x2 = lerp(grad_ab, grad_bb, u);
        return lerp(lerp_x1, lerp_x2, v);
    }

    void randomizeSeed() {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
        init();
    }
};
