#pragma once

#include "core/Loader.h"
#include "renderer/RenderTypes.h"
#include "utils/Types.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/scene.h>

#include <functional>
#include <optional>
#include <vector>

#define DEFAULT_BOUNDING_BOX {glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX)}

typedef struct {
    glm::vec3 min;
    glm::vec3 max;
} BoundingBox;

class Mesh : public ResourceProcessor {
  public:
    Material *material = nullptr;
    glm::vec3 defaultColor;

    Mesh(unsigned int primitiveType = GL_TRIANGLES);
    virtual ~Mesh() {};

    static Mesh *Load(std::string ime);
    static Mesh *Load(std::string ime, glm::vec3 defaultColor);

    void addVertex(float x, float y, float z);
    void addVertex(float x, float y, float z, float r, float g, float b);
    void addVertex(glm::vec3 vrh, glm::vec3 boja);
    void addVertex(float vrh[3], float boja[3]);
    void addVertexStrip(glm::vec3 vrh, glm::vec3 boja); // also adds indices if possible

    void addUV(glm::vec2);
    void addUV(float x, float y);

    void addIndices(unsigned int i1, unsigned int i2, unsigned int i3);
    void addIndices(unsigned int i1, unsigned int i2);
    // template <typename... Args> void addIndices(Args... args);
    // void addIndices(unsigned int i...);
    void addIndices(unsigned int i[]);
    void addIndex(unsigned int i);

    void addNormal(glm::vec3 norm) { addNormal(norm.x, norm.y, norm.z); };
    void addNormal(float i1, float i2, float i3) { normals.insert(normals.end(), {i1, i2, i3}); };

    void calculateAABB(const glm::mat4 &modelMatrix, glm::vec3 &min, glm::vec3 &max);
    void getBoundingBox(glm::vec3 &min, glm::vec3 &max);
    int getPrimitiveType() { return primitiveType; }
    void setPrimitiveType(int type) { primitiveType = type; }

    void applyTransform(glm::mat4 transform);

    void addChangeListener(std::function<void()> f) { listeners.push_back(f); }
    void commit() {
        for (const auto &l : listeners)
            l();
    }
    // void recalculateNormals();

    std::optional<Intersection> findIntersection(glm::vec3 origin, glm::vec3 direction, glm::mat4 matrix);

    glm::vec3 getIndices(int triangleIndex);
    glm::vec3 getVertex(int redniBroj);
    glm::vec3 getColor(int redniBroj);
    glm::mat3 getTriangle(int indeks);
    glm::vec3 getNormal(int redniBroj);

    void setVertex(int indeks, glm::vec3 vertex);
    void setColor(int indeks, glm::vec3 color);
    void setNormal(int indeks, glm::vec3 normal);

    bool hasNormals() { return !normals.empty(); }

    int numberOfVertices() { return vrhovi.size() / 3; }; // normals too if present
    int numberOfIndices() { return indeksi.size() / 3; };
    int numberOfPolygons() { return numberOfIndices(); };

    void removeAllVertices();

    // iterate over vertices
    class VerticesIterator {
      private:
        std::vector<float>::const_iterator current;
        std::vector<float>::const_iterator end_;

      public:
        VerticesIterator(std::vector<float>::const_iterator start, std::vector<float>::const_iterator finish)
            : current(start), end_(finish) {}

        VerticesIterator begin() const { return VerticesIterator(current, end_); }
        VerticesIterator end() const { return VerticesIterator(end_, end_); }

        const float &operator*() const { return *current; }
        VerticesIterator &operator++() {
            ++current;
            return *this;
        }
        bool operator!=(const VerticesIterator &other) const { return current != other.current; }
    };

    VerticesIterator vertices() const { return VerticesIterator(vrhovi.begin(), vrhovi.end()); }

    // TODO: polygons iterator

    // all of these have to be divisible by 3
    std::vector<float> vrhovi;
    std::vector<float> boje;
    std::vector<float> normals;
    std::vector<float> textureCoords; // except this one
    std::vector<unsigned int> indeksi;

    std::vector<unsigned int> textureIndices; // unused

  private:
    GLint primitiveType = GL_TRIANGLES;
    BoundingBox bb = DEFAULT_BOUNDING_BOX;

    std::vector<std::function<void()>> listeners;

    void processResource(std::string name, const aiScene *scene);
    void processMesh(aiMesh *mesh);
    void getDataFromArrays(float *vrhovi, float *boje, int brojVrhova);
};
