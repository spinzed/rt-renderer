#pragma once

#include <assimp/scene.h>

#include <string>

class ResourceProcessor {
  public:
    virtual void processResource(std::string name, const aiScene *resource) = 0;
};

class Object;

class Loader {
  public:
    // deprecated
    static bool LoadResource(std::string name, ResourceProcessor *p, std::string &error);
    static const aiScene *LoadResource(std::string name, std::string &error);

    static void setPath(std::string path);
    static unsigned char *LoadTexture(std::string resourcePath, int &width, int &height, int &nrChannels);
    static void freeResource(unsigned char *data); // only for LoadTexture
    static std::string getFilePath(std::string name);

  private:
    static std::string _path;
};