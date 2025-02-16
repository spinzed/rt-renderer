#pragma once

#include "models/Curve.h"

#include "core/Transform.h"

#include <glm/glm.hpp>
#include <iostream>

class Animation {
  public:
    Curve *curve;
    float current;
    float totalDuration;

    std::function<void(float, float)> handlerOnChange = nullptr; // optional

    Animation(Curve *c, float duration);
    virtual ~Animation() {};

    void addDuration(float duration) {
        current += duration;
        if (current > totalDuration) {
            current = totalDuration;
        }
    }

    virtual void onChange(float current, float totalDuration) {
        if (handlerOnChange) {
            handlerOnChange(current, totalDuration);
        }
    };

    void setOnChange(std::function<void(float, float)> h) { handlerOnChange = h; }

    bool done() { return current >= totalDuration; }

    void reset() { current = 0; }
};

// designed for reusability
class TransformAnimation : public Animation {
  public:
    TransformAnimation(float duration, Transform *t, glm::vec3 scale) : Animation(nullptr, duration) {
        transform = t;
        target = scale;
    }

    static TransformAnimation scale(float duration, Transform *t, glm::vec3 scale) {
        assert(t != nullptr);

        TransformAnimation ta(duration, t, scale);
        ta.setOnChange([&](float current, float totalDuration) {
            Transform *t = ta.transform;
            assert(t != nullptr);

            if (ta.targetSet) {
                t->setScale(ta.initialValue + (ta.target - ta.initialValue) * (current / totalDuration));
            } else {
                ta.initialValue = t->getScale();
                ta.targetSet = true;
            }

            // reset
            if (ta.done()) {
                ta.targetSet = false;
            }
        });
        return ta;
    }
    static TransformAnimation rotate(float duration, Transform *t, glm::vec3 axis, float degrees) {
        assert(t != nullptr);

        TransformAnimation ta(duration, t, axis);
        ta.deg = degrees;
        ta.setOnChange([&](float current, float totalDuration) {
            Transform *t = ta.transform;
            assert(t != nullptr);

            if (ta.targetSet) {
                t->rotate(ta.target, ta.deg * (current / totalDuration) * (1.0f / 60.0f));
            } else {
                ta.initialValue = t->getEulerAngles();
                ta.targetSet = true;
            }

            // reset
            if (ta.done()) {
                ta.targetSet = false;
            }
        });
        return ta;
    }

  private:
    glm::vec3 target;
    float deg;
    glm::vec3 initialValue;
    bool targetSet = false;
    Transform *transform;
};
