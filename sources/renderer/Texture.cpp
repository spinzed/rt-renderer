#include "renderer/Texture.h"

#include "core/Loader.h"
#include "utils/GLDebug.h"

#include <format>
#include <iostream>
#include <stdexcept>

const int formatMap[] = {-1, GL_RED, -1, GL_RGB, GL_RGBA};

std::unordered_map<int, std::vector<int>> fullFormatMatrix = {
    {GL_UNSIGNED_BYTE, {-1, GL_RED, -1, GL_RGB, GL_RGBA}},
    {GL_FLOAT, {-1, GL_RED, -1, GL_RGB32F, GL_RGBA32F}},
};

Texture::Texture(int glType, int width, int height, int channels, bool isDepth) {
    assert(channels == 1 || channels == 3 || channels == 4);

    this->glType = glType;
    this->channels = channels;
    this->isDepth = isDepth;

    GLCheckError();
    glGenTextures(1, &id);
    // glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(glType, id);
    GLCheckError();

    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(glType, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(glType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(glType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLCheckError();

    setSize(width, height);
    GLCheckError();
}

void Texture::setStorage(unsigned int mask) {
    if (width <= 0 || height <= 0 || isDepth) {
        std::cout << "Storage cannot be set in this texture: " << width << " " << height << " " << isDepth << std::endl;
        return;
    }
    int fullPictureFormat = fullFormatMatrix[GL_FLOAT][channels];
    glTexStorage2D(glType, 1, fullPictureFormat, width, height); // allocate storage, for compute
    int mode;
    if (mask == 1) {
        mode = GL_READ_ONLY;
    } else if (mask == 2) {
        mode = GL_WRITE_ONLY;
    } else if (mask == 3) {
        mode = GL_READ_WRITE;
    } else {
        throw std::invalid_argument(std::format("invalid mask in texture storage init: {}", std::to_string(mask)));
    }
    glBindImageTexture(0, id, 0, GL_FALSE, 0, mode, fullPictureFormat);
}

void Texture::setSize(int w, int h) {
    width = w;
    height = h;
    // setData<float>(4, NULL);                                              // not needed, might disable
    //  glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture
}

int Texture::totalSize() { return width * height * channels; }

int Texture::sizeInMemory() { return totalSize() * sizeof(float); }

void Texture::dumpData(float *output) {
    GLCheckError();
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    use(0);
    glBufferData(GL_PIXEL_PACK_BUFFER, sizeInMemory(), nullptr, GL_STREAM_READ);
    GLCheckError();
    int pictureFormat = isDepth ? GL_DEPTH_COMPONENT : formatMap[channels];
    glGetTexImage(GL_TEXTURE_2D, 0, pictureFormat, GLtype<float>::value, 0); // GL_FLOAT or GL_UNSIGNED_BYTE
    GLCheckError();
    glFinish();
    void *ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    GLCheckError();
    if (ptr) {
        memcpy(output, ptr, sizeInMemory());
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        GLCheckError();
    }
    GLCheckError();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glDeleteBuffers(1, &pbo);
    GLCheckError();
}

void Texture::use(int textureID) {
    GLCheckError();
    glActiveTexture(GL_TEXTURE0 + textureID);
    glBindTexture(glType, id);
    GLCheckError();
}

Texture *Texture::Load(std::string resourcePath) {
    int width, height, nrChannels;
    unsigned char *data = Loader::LoadTexture(resourcePath, width, height, nrChannels);

    Texture *tx = new Texture(GL_TEXTURE_2D, width, height);
    assert(tx != nullptr);

    tx->setData(data);
    Loader::freeResource(data);

    return tx;
}

// TODO: move to Loader.h (or at least move majority of logic there)
Texture *Texture::Load(std::string resourceName, std::string fileName) { return Load(resourceName + "/" + fileName); }
