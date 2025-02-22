#pragma once

#include "models/Raster.h"
#include "renderer/Texture.h"

#include <memory>
#include <vector>

class Cubemap : public Texture {
  public:
    Cubemap(int width, int height, int channels, bool isDepth);

    // use these instead of Texture::setData for setting cubemap data
    template <typename T> void setCubemapData(int side, Raster<T> *raster);
    template <typename T> void setCubemapData(int side, T *data);

    static Cubemap Load(std::vector<std::string> faces);
    static Cubemap Load(std::string resourceName);

  private:
    std::vector<std::unique_ptr<Texture>> textures;
};
