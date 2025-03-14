#pragma once

#include "models/Raster.h"
#include "objects/Object.h"
#include "renderer/Texture.h"

class FullscreenTexture : public Object {
  public:
    FullscreenTexture(std::string name, std::string shaderName);
    ~FullscreenTexture();
    void setTexture(Texture *texture);
    template <typename T> void loadRaster(Raster<T> *raster);

    using Object::render;

    Texture *texture = nullptr;
};

template <typename T> void FullscreenTexture::loadRaster(Raster<T> *raster) {
    assert(texture != nullptr);
    shader->use();
    texture->setData(raster);
}
