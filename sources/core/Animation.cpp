#include "core/Animation.h"

Animation::Animation(Curve *c, float duration) {
    curve = c;
    totalDuration = duration;
    current = 0;
}
