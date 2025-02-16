#pragma once

#include <core/Animation.h>

#include <vector>

class Animator {
  public:
    static void registerAnimation(Animation *anim);
    static void unregisterAnimation(Animation *anim);
    static void passTime(float time); // in seconds

  private:
    Animator();

    static std::vector<Animation *> _animations;
};