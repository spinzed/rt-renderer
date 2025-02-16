#pragma once

#include "objects/PointCloud.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <functional>
#include <vector>

struct ParticleUpdate {
    float current;
    float duration;
    float deltaTime;
    float t;
};

enum ParticleClusterBehavior {
    ONE_TIME,
    ONE_TIME_THEN_DESTROY,
    REPEAT,
    REPEAT_ALTERNATE,
};

class ParticleCluster : public PointCloud {
  public:
    float elapsed = 0;
    float totalElapsed = 0;
    float duration = -1; // in ms
    ParticleClusterBehavior behavior = ParticleClusterBehavior::REPEAT;
    int direction = 1; // 1 == forward, -1 == reverse
    int n;

    ParticleCluster(int n) { this->n = n; }

    // void init() {
    //     if (onInitFunc) {
    //         onInitFunc(this, generateUpdate(0, 0));
    //     }
    //     if (onResetFunc) {
    //         onResetFunc(this, generateUpdate(0, 0));
    //     }
    //     commit();
    // }

    void addDuration(float d) {
        elapsed += d;
        if (elapsed > duration) {
            elapsed = duration;
        }
        totalElapsed += d;
    }
    void setDuration(float d) { duration = d; }

    void setBehavior(ParticleClusterBehavior b) { behavior = b; }
    bool isAutoremove() { return behavior == ONE_TIME_THEN_DESTROY; }
    bool isRepeating() {
        return behavior == ParticleClusterBehavior::REPEAT || behavior == ParticleClusterBehavior::REPEAT_ALTERNATE;
    }

    void setOnInit(std::function<void(ParticleCluster *, ParticleUpdate)> handler) { onInitFunc = handler; }
    void setOnChange(std::function<void(ParticleCluster *, ParticleUpdate)> handler) { onChangeFunc = handler; }
    void setOnReset(std::function<void(ParticleCluster *, ParticleUpdate)> handler) { onResetFunc = handler; }

    void resetElapsed() { elapsed = 0; }

    bool hasMaxDuration() { return duration >= 0; }
    bool done() { return hasMaxDuration() && elapsed >= duration; };

    std::function<void(ParticleCluster *, ParticleUpdate)> onInitFunc = nullptr;   // ran only the first time
    std::function<void(ParticleCluster *, ParticleUpdate)> onChangeFunc = nullptr; // ran on every change/time pass
    std::function<void(ParticleCluster *, ParticleUpdate)> onResetFunc = nullptr;  // ran first time and on reset

    ParticleUpdate generateUpdate(float t, float dt) {
        ParticleUpdate pu = {.current = elapsed, .duration = duration, .deltaTime = dt, .t = direction > 0 ? t : 1 - t};
        return pu;
    }
};

// TODO
class ParticleSpawner {
  public:
    ParticleCluster *cluster;

    ParticleSpawner(ParticleCluster *pc) { cluster = pc; }
};

class ParticleSystem {
  public:
    static void registerCluster(ParticleCluster *cluster) {
        clusters.push_back(cluster);
        if (cluster->onInitFunc) {
            cluster->onInitFunc(cluster, cluster->generateUpdate(0, 0));
        }
        if (cluster->onResetFunc) {
            cluster->onResetFunc(cluster, cluster->generateUpdate(0, 0));
        }
        cluster->commit();
    };

    static void passTime(float time) {
        for (auto i = clusters.begin(); i != clusters.end(); ++i) {
            ParticleCluster *a = *i;

            a->addDuration(time);

            if (a->done()) {
                if (a->isRepeating()) {
                    if (a->behavior == ParticleClusterBehavior::REPEAT_ALTERNATE) {
                        if (a->direction < 0 && a->onResetFunc) {
                            a->onResetFunc(a, a->generateUpdate(0, 0));
                        }
                        a->direction = -a->direction;
                    } else {
                        if (a->onResetFunc) {
                            a->onResetFunc(a, a->generateUpdate(0, 0));
                        }
                    }
                    a->resetElapsed();
                } else if (a->isAutoremove()) {
                    clusters.erase(i--);
                }
            }

            if (a->onChangeFunc) {
                a->onChangeFunc(a, a->generateUpdate(a->elapsed / a->duration, time));
            }
            a->commit();
        }
    }

    inline static std::vector<ParticleCluster *> clusters;

  private:
    ParticleSystem();
};
