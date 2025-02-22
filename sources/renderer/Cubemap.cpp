#include "renderer/Cubemap.h"
#include "renderer/Texture.h"
#include "utils/GLDebug.h"
#include "core/Loader.h"

#include <stb_image.h>

Cubemap::Cubemap(int width, int height, int channels = 3, bool isDepth = false)
    : Texture::Texture(GL_TEXTURE_CUBE_MAP, width, height, channels, isDepth) {
    GLCheckError();
    for (int i = 0; i < 6; i++) {
        setCubemapData<unsigned char>(i, NULL);
    }
    GLCheckError();
}

Cubemap Cubemap::Load(std::string resourceName) {
    return Cubemap::Load({
        resourceName + "/right.png",
        resourceName + "/left.png",
        resourceName + "/top.png",
        resourceName + "/bottom.png",
        resourceName + "/front.png",
        resourceName + "/back.png",
    });
}

Cubemap Cubemap::Load(std::vector<std::string> faces) {
    int width, height, nrChannels;
    int i = 0;
    Cubemap cb(0, 0);
    for (const auto &imagePath : faces) {
        unsigned char *data = Loader::LoadTexture(imagePath, width, height, nrChannels);

        cb.use(0);
        if (i == 0)
            cb.setSize(width, height);
        cb.setCubemapData(i, data);
        Loader::freeResource(data);
        ++i;
    }
    return cb;
}

template <typename T> void Cubemap::setCubemapData(int side, Raster<T> *raster) {
    if (!raster)
        return;

    assert ((int) raster->channels == channels);
    setCubemapData(side, raster->get());
}

template <typename T> void Cubemap::setCubemapData(int side, T *data) {
    setTextureData(1, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, data);
}
