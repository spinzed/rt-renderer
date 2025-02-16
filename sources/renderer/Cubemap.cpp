#include "renderer/Cubemap.h"
#include "renderer/Texture.h"
#include "utils/GLDebug.h"
#include "core/Loader.h"

#include <stb_image.h>

Cubemap::Cubemap(int width, int height, bool isDepth = false)
    : Texture::Texture(GL_TEXTURE_CUBE_MAP, width, height, isDepth) {
    GLCheckError();
    for (int i = 0; i < 6; i++) {
        setCubemapData<unsigned char>(i, 3, NULL);
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
        cb.setCubemapData(i, nrChannels, data);
        Loader::freeResource(data);
        ++i;
    }
    return cb;
}

template <typename T> void Cubemap::setCubemapData(int side, Raster<T> *raster) {
    setCubemapData(side, raster->channels, raster->get());
}

template <typename T> void Cubemap::setCubemapData(int side, int channels, T *data) {
    setTextureData(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, channels, data);
}
