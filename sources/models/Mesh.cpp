#include "models/Mesh.h"

#include "glm/geometric.hpp"
#include "core/Loader.h"
#include "renderer/Texture.h"
#include "utils/mtr.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <assimp/scene.h>

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <vector>

#define DEFAULT_COLOR glm::vec3(1, 1, 1)

#define DEBUG_WIREFRAME 0
#define DEBUG_NOCOLOR 0

Mesh::Mesh(unsigned int primitiveType) {
    material = new Material();
    defaultColor = DEFAULT_COLOR;
    setPrimitiveType(primitiveType);
}

Mesh *Mesh::Load(std::string ime) { return Mesh::Load(ime, DEFAULT_COLOR); }

Mesh *Mesh::Load(std::string name, glm::vec3 defaultColor) {
    Mesh *mesh = new Mesh(); // TODO: make non-owned
    mesh->defaultColor = defaultColor;
    std::string error;
    bool ok = Loader::LoadResource(name, (ResourceProcessor *)mesh, error);
    if (!ok) {
        std::cout << "Error importing " << name << ": " << error << std::endl;
    }
    mesh->commit();
    return mesh;
}

void Mesh::getDataFromArrays(float *vrhovi, float *boje, int brojVrhova) {
    for (int i = 0; i < brojVrhova; i++) {
        float *vrh = &vrhovi[3 * i];
        float *boja = &boje[3 * i];

        addVertex(vrh, boja);

        if (i > 2) {
            addIndices(i - 2, i - 1, i);
        }
    }
}

void copyColor(aiColor3D color, glm::vec3 &dest) {
    dest.r = color.r;
    dest.g = color.g;
    dest.b = color.b;
}

// sugavi assimp
std::string fixPath(const std::string &path) {
    std::string linuxPath = path;
    std::replace(linuxPath.begin(), linuxPath.end(), '\\', '/');
    return linuxPath;
}

// TODO: move into the importing subsystem
void Mesh::processResource(std::string name, const aiScene *scene) {
    if (!scene->HasMeshes())
        return;

    if (scene->mNumMeshes == 0) {
        std::cout << "Detected no meshes for " << name << std::endl;
    } else if (scene->mNumMeshes > 1) {
        std::cout << "Merged all " << scene->mNumMeshes << " meshes of asset " << name << " into one" << std::endl;
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        Mesh::processMesh(scene->mMeshes[i]);
    }

    // materials
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        if (i != scene->mNumMaterials - 1)
            continue;
        aiColor3D ambientK, diffuseK, specularK, reflectiveK, emissiveK;
        float shininessK;
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, ambientK);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseK);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, specularK);
        scene->mMaterials[i]->Get(AI_MATKEY_SHININESS, shininessK);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_REFLECTIVE, reflectiveK);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveK);
        copyColor(ambientK, material->colorAmbient);
        copyColor(diffuseK, material->colorDiffuse);
        copyColor(specularK, material->colorSpecular);
        copyColor(reflectiveK, material->colorReflective);
        copyColor(emissiveK, material->colorEmissive);

        aiMaterial *mat = scene->mMaterials[i];
        aiString materialName;
        aiReturn ret;

        ret = mat->Get(AI_MATKEY_NAME, materialName);
        if (ret != AI_SUCCESS)
            materialName = "";

        // Diffuse maps
        int numTextures = mat->GetTextureCount(aiTextureType_DIFFUSE); // Amount of diffuse textures
        aiString textureName; // Filename of the texture using the aiString assimp structure

        if (numTextures > 0 && AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureName)) {
            // why the hell are there windows delimiters???
            // material->texture = Loader::LoadTexture(name, fixPath(textureName.data)); // TODO: katastrofa
            auto tx = Texture::Load(name, fixPath(textureName.data)); // TODO: memory leak
            material->texture = tx->id;
        }

        if (numTextures > 1) {
            std::cout << "warning: Detected " << numTextures << " textures, but only one is read" << std::endl;
        }
    }
}

void Mesh::processMesh(aiMesh *mesh) {
    if (!mesh->HasFaces())
        return;

    int vStart = numberOfVertices();

    // vertices
    vrhovi.reserve(mesh->mNumVertices * 3);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        float vertex[] = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

        if (mesh->HasVertexColors(i)) {
            float color[] = {mesh->mColors[i]->r, mesh->mColors[i]->g, mesh->mColors[i]->b, mesh->mColors[i]->a};
            addVertex(vertex, color);
        } else {
            addVertex(vertex, glm::value_ptr(defaultColor));
        }
    }

    // indices
    indeksi.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        assert(mesh->mFaces[i].mNumIndices == 3);
        unsigned int *ind = mesh->mFaces[i].mIndices;
        ind[0] += vStart;
        ind[1] += vStart;
        ind[2] += vStart;
        addIndices(ind);
    }

    // normals
    if (mesh->HasNormals()) {
        indeksi.reserve(mesh->mNumVertices * 3);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            normals.insert(normals.end(), {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z});
        }
    }

    int tStart = textureCoords.size() / 2;

    // texturesCoords
    textureCoords.reserve(mesh->mNumVertices * 2);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        if (mesh->HasTextureCoords(0)) {
            textureCoords.push_back(mesh->mTextureCoords[0][i].x + tStart);
            textureCoords.push_back(mesh->mTextureCoords[0][i].y + tStart);
        } else {
            textureCoords.push_back(0.0f);
            textureCoords.push_back(0.0f);
        }
    }

    // textureIndices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            textureIndices.push_back(face.mIndices[j]);
        }
    }
}

void Mesh::calculateAABB(const glm::mat4 &modelMatrix, glm::vec3 &min, glm::vec3 &max) {
    glm::vec3 minCorner(FLT_MAX);
    glm::vec3 maxCorner(-FLT_MAX);

    for (int i = 0; i < numberOfVertices(); ++i) {
        glm::vec4 transformedVertex = modelMatrix * glm::vec4(getVertex(i), 1.0f);
        glm::vec3 transformedVertex3(transformedVertex);

        minCorner = glm::min(minCorner, transformedVertex3);
        maxCorner = glm::max(maxCorner, transformedVertex3);
    }

    min = minCorner;
    max = maxCorner;
}

void Mesh::getBoundingBox(glm::vec3 &min, glm::vec3 &max) {
    min = bb.min;
    max = bb.max;
}

void Mesh::applyTransform(glm::mat4 transform) {
    for (int i = 0; i < numberOfVertices(); i++) {
        setVertex(i, transform * glm::vec4(getVertex(i), 1));
    }
    if (hasNormals()) {
        for (int i = 0; i < numberOfVertices(); i++) {
            setNormal(i, transform * glm::vec4(getNormal(i), 1));
        }
    }
}

// needs fixin (normal per vertex instead of polygon)
// void Mesh::recalculateNormals() {
//    normals.clear();
//    normals.reserve(numberOfPolygons());
//
//    for (int i = 0; i < numberOfPolygons(); i++) {
//        glm::mat3 triangles = getTriangle(i);
//        glm::vec3 normal = glm::cross(triangles[1] - triangles[0], triangles[2] - triangles[0]);
//        normals[i] = normal[0];
//        normals[i + 1] = normal[1];
//        normals[i + 2] = normal[2];
//    }
//}

void Mesh::addVertex(float x, float y, float z) { addVertex(x, y, z, defaultColor.x, defaultColor.y, defaultColor.z); }

void Mesh::addVertex(float vrh[3], float boja[3]) { addVertex(vrh[0], vrh[1], vrh[2], boja[0], boja[1], boja[2]); }

void Mesh::addVertex(glm::vec3 vrh, glm::vec3 boja) { addVertex(vrh[0], vrh[1], vrh[2], boja[0], boja[1], boja[2]); }

// Doesn't refresh update the buffer data and doesn't add new indices
// every vertex-adding must ultimately call this function
void Mesh::addVertex(float x, float y, float z, float a, float b, float c) {
    vrhovi.insert(vrhovi.end(), {x, y, z});

    // a = (float) std::rand() / RAND_MAX, b = (float) std::rand() / RAND_MAX, c = (float) std::rand() / RAND_MAX;
#if DEBUG_NOCOLOR
    boje.insert(boje.end(), {1, 1, 1});
#else
    boje.insert(boje.end(), {a, b, c});
#endif

    if (bb.min.x > x)
        bb.min.x = x;
    if (bb.min.y > y)
        bb.min.y = y;
    if (bb.min.z > z)
        bb.min.z = z;
    if (bb.max.x < x)
        bb.max.x = x;
    if (bb.max.y < y)
        bb.max.y = y;
    if (bb.max.z < z)
        bb.max.z = z;
}

void Mesh::addVertexStrip(glm::vec3 vrh, glm::vec3 boja) {
    addVertex(vrh, boja);
    if (numberOfVertices() > 2) {
        addIndices(numberOfVertices() - 3, numberOfVertices() - 2, numberOfVertices() - 1);
    }
}

void Mesh::addUV(glm::vec2 uv) { addUV(uv.x, uv.y); }

void Mesh::addUV(float u, float v) { textureCoords.insert(textureCoords.end(), {u, v}); }

void Mesh::addIndices(unsigned int i1, unsigned int i2) { indeksi.insert(indeksi.end(), {i1, i2}); }

// template <typename... Args> void Mesh::addIndices(Args... args) { (indeksi.push_back(args), ...); }

// void Mesh::addIndices(unsigned int i...) {
//     indeksi.insert(indeksi.end(), i);
// }

void Mesh::addIndices(unsigned int i[]) { addIndices(i[0], i[1], i[2]); }

void Mesh::addIndex(unsigned int i) { indeksi.push_back(i); }

// every index-adding must ultimately call this function
void Mesh::addIndices(unsigned int i1, unsigned int i2, unsigned int i3) {
    indeksi.insert(indeksi.end(), {i1, i2, i3});
}

std::optional<Intersection> Mesh::findIntersection(glm::vec3 origin, glm::vec3 direction, glm::mat4 matrix) {
    Intersection p;

    float t, u, v;
    bool intersected;

    for (unsigned int i = 0; i < indeksi.size() / 3; ++i) {
        glm::mat3 vrhovi = getTriangle(i);

        // TODO: matrix calculation here can be avoided
        glm::vec3 vrh0 = matrix * glm::vec4(vrhovi[0], 1);
        glm::vec3 vrh1 = matrix * glm::vec4(vrhovi[1], 1);
        glm::vec3 vrh2 = matrix * glm::vec4(vrhovi[2], 1);

        intersected = mtr::intersectLineAndTriangle(origin, direction, vrh0, vrh1, vrh2, &t, &u, &v);

        if (intersected) {
            p.t = t;
            p.point = origin + t * direction;
            glm::vec3 indices = getIndices(i);
            p.color = getColor(indices[0]);
            p.normal = glm::normalize(glm::cross(vrh1 - vrh0, vrh2 - vrh0));
            return p;
        }
    }
    return std::nullopt;
}

glm::vec3 Mesh::getIndices(int triangleIndex) {
    assert((unsigned int)triangleIndex < indeksi.size() / 3);

    int start = 3 * triangleIndex;
    int i0 = indeksi[start];
    int i1 = indeksi[start + 1];
    int i2 = indeksi[start + 2];

    return glm::vec3(i0, i1, i2);
}

glm::mat3 Mesh::getTriangle(int indeks) {
    glm::vec3 indices = getIndices(indeks);
    return glm::mat3(getVertex(indices[0]), getVertex(indices[1]), getVertex(indices[2]));
}

glm::vec3 Mesh::getVertex(int indeks) {
    assert((unsigned int)indeks < vrhovi.size() / 3);

    int start = 3 * indeks;
    glm::vec3 a = glm::vec3(vrhovi[start], vrhovi[start + 1], vrhovi[start + 2]);

    return a;
}

void Mesh::setVertex(int indeks, glm::vec3 vertex) {
    assert((unsigned int)indeks < vrhovi.size() / 3);

    int start = 3 * indeks;
    vrhovi[start] = vertex[0];
    vrhovi[start + 1] = vertex[1];
    vrhovi[start + 2] = vertex[2];
}

void Mesh::setColor(int indeks, glm::vec3 boja) {
    assert((unsigned int)indeks < boje.size() / 3);

    int start = 3 * indeks;
    boje[start] = boja[0];
    boje[start + 1] = boja[1];
    boje[start + 2] = boja[2];
}

void Mesh::setNormal(int indeks, glm::vec3 normal) {
    assert((unsigned int)indeks < normals.size() / 3);

    int start = 3 * indeks;
    normals[start] = normal[0];
    normals[start + 1] = normal[1];
    normals[start + 2] = normal[2];
}

glm::vec3 Mesh::getColor(int indeks) {
    assert((unsigned int)indeks < boje.size() / 3);

    int start = 3 * indeks;
    glm::vec3 a = glm::vec3(boje[start], boje[start + 1], boje[start + 2]);

    return a;
}

glm::vec3 Mesh::getNormal(int indeks) {
    assert((unsigned int)indeks < normals.size() / 3);

    int start = 3 * indeks;
    float v0 = normals[start];
    float v1 = normals[start + 1];
    float v2 = normals[start + 2];

    return glm::vec3(v0, v1, v2);
}

void Mesh::removeAllVertices() {
    vrhovi.clear();
    boje.clear();
    indeksi.clear();
    normals.clear();
    textureCoords.clear();
    textureIndices.clear();
    bb = DEFAULT_BOUNDING_BOX;
}
