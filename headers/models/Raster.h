#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

template <typename T> class Raster {
  public:
    int width, height;
    float *raster;
    Raster(int width, int height);
    Raster(int width, int height, unsigned int channels);
    ~Raster();
    void resize(int width, int height);
    void reset();
    glm::vec3 getFragmentColor(int x, int y);
    void setFragmentColor(int x, int y, glm::vec3 boja);
    float *get();

    unsigned int channels;

  private:
    void init(int width, int height, unsigned int channels);
    void set(int width, int height);
    void del();
};

template <typename T> Raster<T>::Raster(int width, int height, unsigned int channels) { init(width, height, channels); }

template <typename T> Raster<T>::Raster(int width, int height) { init(width, height, 3); }

template <typename T> void Raster<T>::init(int width, int height, unsigned int channels) {
    assert(channels == 1 || channels == 3 || channels == 4);
    this->channels = channels;
    set(width, height);
}

template <typename T> Raster<T>::~Raster() { del(); }

template <typename T> void Raster<T>::resize(int width, int height) {
    del();
    set(width, height);
}

template <typename T> void Raster<T>::set(int width, int height) {
    this->width = width;
    this->height = height;
    raster = static_cast<float *>(calloc(width * height * channels, sizeof(T)));
}

template <typename T> void Raster<T>::reset() {
    std::memset(reinterpret_cast<void *>(raster), 0, width * height * channels * sizeof(T));
}

template <typename T> float *Raster<T>::get() { return raster; }

template <typename T> glm::vec3 Raster<T>::getFragmentColor(int x, int y) {
    // assert((x >= 0 && x < _width) || (y >= 0 && y < _height));
    int h = height - 1 - y;
    int offset = h * width * 3 + x * 3;
    float r = raster[offset];
    float g = raster[offset + 1];
    float b = raster[offset + 2];
    return glm::vec3(r, g, b);
}

template <typename T> void Raster<T>::setFragmentColor(int x, int y, glm::vec3 boja) {
    assert((x >= 0 && x < width) || (y >= 0 && y < height));

    int h = height - 1 - y;
    int offset = h * width * 3 + x * 3;
    raster[offset] = boja.x;
    raster[offset + 1] = boja.y;
    raster[offset + 2] = boja.z;
}

template <typename T> void Raster<T>::del() { free(raster); }
