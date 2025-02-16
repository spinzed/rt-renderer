#pragma once

#include <functional>

class Object;

class Behavior {
  public:
    bool initialized = false;
    Object *object;

    virtual ~Behavior() {};

    virtual void Init(Object *o) { (void)o; }
    virtual void Update(Object *o, float deltaTime) {
        (void)o;
        (void)deltaTime;
    }
    virtual void Destroy(Object *o) { (void)o; }
};

class FunctionalBehavior : public Behavior {
  public:
    std::function<void(Object *)> onInit;
    std::function<void(Object *, float)> onUpdate;
    std::function<void(Object *)> onDestroy;

    virtual void Init(Object *o) {
        if (onInit) {
            onInit(o);
        }
    };
    virtual void Update(Object *o, float deltaTime) {
        if (onUpdate) {
            onUpdate(o, deltaTime);
        }
    };
    virtual void Destroy(Object *o) {
        if (onDestroy) {
            onDestroy(o);
        }
    }
};
