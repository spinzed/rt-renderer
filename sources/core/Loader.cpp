#include "core/Loader.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <iostream>

std::string Loader::_path = "";

void Loader::setPath(std::string path) { _path = path; };

unsigned char *Loader::LoadTexture(std::string resourcePath, int &width, int &height, int &nrChannels) {
    std::string finalPath = _path + "/" + resourcePath;
    // int width, height, nrChannels;
    unsigned char *data = stbi_load(finalPath.c_str(), &width, &height, &nrChannels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << finalPath << std::endl;
        assert(false);
    }

    return data;

    // Texture<unsigned char> tx(width, height);
    // tx.setData(nrChannels, data);
    // stbi_image_free(data);

    // return tx;
}

std::string Loader::getFilePath(std::string name) { return _path + "/" + name; }

bool Loader::LoadResource(std::string name, ResourceProcessor *processor, std::string &error) {
    std::string objPath = getFilePath(name + "/" + name + ".obj");

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        objPath.c_str(), aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                             aiProcess_SortByPType | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene) {
        error = importer.GetErrorString();
        return false;
    }

    processor->processResource(name, scene);

    return true;
}

const aiScene *Loader::LoadResource(std::string name, std::string &error) {
    std::string objPath = getFilePath(name + "/" + name + ".obj");

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        objPath.c_str(), aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                             aiProcess_SortByPType | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene) {
        error = importer.GetErrorString();
    }
    return scene;
}

void Loader::freeResource(unsigned char *data) { stbi_image_free(data); }