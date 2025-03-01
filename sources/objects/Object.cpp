#include "objects/Object.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "assimp/scene.h"

#include <iostream>

Object::Object(std::string name, std::string type) : name(name), type(type) {}

void Object::render() {
    if (shader && renderable) {
        shader->use();
        renderable->render();
    }
}

void Object::render(Shader *s) {
    Shader *oldShader = shader;
    shader = s;
    render();
    shader = oldShader;
}

glm::mat4 Object::getModelMatrix() {
    return parent != nullptr ? parent->getModelMatrix() * transform.getMatrix() : transform.getMatrix();
}
Transform *Object::getTransform() { return &transform; };

Transform Object::getFullTransform() { return Transform(getModelMatrix()); };

void Object::addBehavior(Behavior *b) {
    behaviors.emplace_back(b);
    b->object = this;
}

void Object::removeBehavior(Behavior *b) {
    behaviors.erase(std::remove(behaviors.begin(), behaviors.end(), b), behaviors.end());
}

void Object::addChild(Object *o) {
    children.emplace_back(o);
    o->parent = this;
    o->rootParent = rootParent != nullptr ? rootParent : this;
}

void Object::removeChild(Object *o) {
    children.erase(std::remove(children.begin(), children.end(), o), children.end());
    o->parent = nullptr;
    o->rootParent = nullptr;
}

Object *Object::GetChild(std::string name) {
    for (Object *child : children) {
        if (child->name == name) {
            return child;
        }
    }
    return nullptr;
}

void Object::applyTransform() {
    if (mesh) {
        mesh->applyTransform(transform.getMatrix());
        transform.setMatrix(glm::mat4(1));
    }
}

void Object::commit(bool force) {
    if (force)
        mesh->commit();

    uncommited = !force;
}
