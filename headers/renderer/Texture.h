#pragma once

#include "models/Raster.h"
#include "utils/GLDebug.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <string>
#include <unordered_map>
#include <vector>

template <typename T> struct GLtype;

template <> struct GLtype<float> {
    static constexpr int value = GL_FLOAT;
};

template <> struct GLtype<unsigned char> {
    static constexpr int value = GL_UNSIGNED_BYTE;
};

// channels -> format
extern const int formatMap[];

// type, channels -> format
extern std::unordered_map<int, std::vector<int>> fullFormatMatrix;

class Texture {
  public:
    GLuint id;
    int glType;

    Texture(int glType, int width, int height, int channel = 3, bool isDepth = false);

    void use(int textureID);
    void setSize(int width, int height);
    int totalSize();
    int sizeInMemory();
    void dumpData(float *output);

    template <typename T> void setData(Raster<T> *raster);
    template <typename T> void setData(T *data);
    template <typename T> void getData(std::vector<T> &data);

    void setStorage(unsigned mask);

    static Texture *Load(std::string resourceName, std::string fileName);
    static Texture *Load(std::string resourcePath);

    int width;
    int height;
    int channels = -1;

  protected:
    // also accepts type so it can also be used for setting cubemap sides
    template <typename T> void setTextureData(int position, int glTextureType, T *data);

    void generateMipmaps() {
        GLCheckError();
        glGenerateMipmap(glType); // triba maknit ka opciju
        GLCheckError();
    }

  private:
    bool isDepth = false;
};

template <typename T> void Texture::setData(Raster<T> *raster) {
    if (!raster)
        return;

    assert((int) raster->channels == channels);
    setData(raster->get());
}

template <typename T> void Texture::setData(T *data) {
    setTextureData(0, glType, data);
    generateMipmaps();
}

// 0 is reserved for temporary texture2d and 1 for cubemap
template <typename T> void Texture::setTextureData(int position, int glTextureType, T *data) {
    if (glTextureType == GL_TEXTURE_CUBE_MAP)
        return;

    use(position);
    int glType = GLtype<T>::value;
    int pictureFormat = isDepth ? GL_DEPTH_COMPONENT : formatMap[channels];
    int fullPictureFormat = isDepth ? GL_DEPTH_COMPONENT : fullFormatMatrix[glType][channels];

    GLCheckError();
    glTexImage2D(glTextureType, 0, fullPictureFormat, width, height, 0, pictureFormat, glType, (void *)data);
    GLCheckError();
}

template <typename T> void Texture::getData(std::vector<T> &data) {
    glBindTexture(GL_TEXTURE_2D, id);

    // Create a buffer to hold the data
    data.resize(width * height * channels);

    // Read the texture data
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data.data());
}
