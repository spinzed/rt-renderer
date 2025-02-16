#include <core/Animation.h>
#include <core/Animator.h>

#include <algorithm>
#include <vector>

std::vector<Animation *> Animator::_animations;

void Animator::registerAnimation(Animation *anim) {
    _animations.push_back(anim);
    anim->reset(); // in case it was used previously
    anim->onChange(0, anim->totalDuration);
};

void Animator::unregisterAnimation(Animation *anim) {
    _animations.erase(std::remove(_animations.begin(), _animations.end(), anim), _animations.end());
};

void Animator::passTime(float time) {
    for (auto i = _animations.begin(); i != _animations.end(); ++i) {
        Animation *a = *i;

        a->addDuration(time);
        a->onChange(a->current, a->totalDuration);
        if (a->done()) {
            _animations.erase(i--);
        }
    }
}
