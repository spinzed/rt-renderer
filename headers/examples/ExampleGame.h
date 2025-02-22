#pragma once

#include "core/Engine.h"
#include "core/Input.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"
#include "utils/PerlinNoise.h"
#include "utils/Timer.h"
#include "utils/mtr.h"

#include <string>
#include <mutex>

class ExampleGame {
  public:
    int width = 1000, height = 1000;
    float moveSensitivity = 10, sprintMultiplier = 5, mouseSensitivity = 0.15f;

    Engine *app;

    PerlinNoise perlin;

    Shader *genShader;
    Texture *genIn;
    Texture *genOut;

    inline static bool showCulled = false;

    int run(std::string dir);
    void KeyCallback(InputGlobalListenerData event);
    void cursorPositionCallback(WindowCursorEvent event);

    class Field : public Behavior {
      public:
        struct Point {
            glm::vec2 location;
            float value;
            int type; // 1 out, 2 edge, 3 in
            bool checked;
            bool stripped;
        };

        PerlinNoise perlin;

        Shader *genShader;
        Texture *genIn;
        Texture *genOut;

        glm::vec2 _min = glm::vec2(0);
        glm::vec2 _max = glm::vec2(25);
        glm::vec2 scale = glm::vec2(20, 3);
        glm::vec2 resolution = glm::vec2(1000, 1000);
        std::vector<std::vector<Point>> points;
        std::vector<glm::vec2> finalPoints;
        std::mutex finalPointsGuard;

        float threshold = 0.3f;
        bool suppressGeneration = false;
        bool useCPU = true;
        int stripEdgeMask = 0;

        std::mutex generateMutex;
        bool busy = false;

        virtual void Init(Object *object) {
            PointCloud *perlinCloud = dynamic_cast<PointCloud *>(object);
            perlinCloud->setPointSize(2.0f);
        }

        virtual void reset() {
            assert(object != nullptr);
            PointCloud *perlinCloud = dynamic_cast<PointCloud *>(object);

            perlinCloud->reset();
            points.clear();
            points.resize(resolution.x);
            finalPoints.clear();
        }

        virtual void generate(float min_, float max_) {
            Timer t = Timer::start();

            if (useCPU) {
                generateCPU(min_, max_);
            } else {
                generateGPU(min_, max_);
            }

            t.printElapsed("Generating done in: $");
        }

        virtual void generateGPU(float min_, float max_) {
            (void)min_;
            (void)max_;
            if (!genOut) {
                genShader = Shader::LoadCompute("generator");
                // genIn = new Texture(GL_TEXTURE_2D, resolution.x, resolution.y, 0, 4);
                // genIn->setStorage(0x01);
                genOut = new Texture(GL_TEXTURE_2D, resolution.x, resolution.y, 0, 4);
                genOut->setStorage(0x01);
            }

            // genShader->use();
            // genShader->setVectorInt("boundMin", _min);
            // genShader->setVectorInt("boundMax", _max);
            // genShader->setVectorInt("scale", scale);
            // genShader->setVectorInt("resolution", resolution);
            // genShader->setFloat("threshold", threshold);
            // genShader->setInt("stripEdgeMask", stripEdgeMask);
            // genShader->compute(resolution.x, resolution.y);
            //
            std::vector<float> out;
            genOut->getData(out);

            std::cout << out[0] << std::endl;
        }

        virtual void generateCPU(float min_, float max_) {
            assert(object != nullptr);
            _min.x = min_;
            _max.x = max_;

            PointCloud *perlinCloud = dynamic_cast<PointCloud *>(object);

            reset();

            if (suppressGeneration)
                return;

            glm::vec2 diff = _max / scale - _min / scale;

            // generate values
            for (int x = 0; x < resolution.x; x++) {
                points[x].resize(resolution.y);
                float xCoord = _min.x / scale.x + diff.x / resolution.x * x;

                for (int y = 0; y < resolution.y; y++) {
                    float yCoord = _min.y / scale.y + diff.y / resolution.y * y;
                    points[x][y] = {{xCoord, yCoord}, perlin.noise(xCoord, yCoord), 1, 0, 0};
                }
            }

            // generate the edges
            for (int x = 0; x < resolution.x; x++) {
                for (int y = 0; y < resolution.y; y++) {
                    Point &p = points[x][y];
                    p.type = (p.value > threshold && findOutOfRangeNeighbors(x, y)) ? 2 : 1;
                }
            }

            // clamp the unfinished edges
            for (int x = 0; x < resolution.x; x++) {
                for (int y = 0; y < resolution.y; y++) {
                    Point &p = points[x][y];
                    p.checked = true;

                    // skip if not applicable
                    if (p.value < threshold || p.type != 2)
                        continue;

                    if (((stripEdgeMask & 0b1000) && (x == 0)) ||
                        ((stripEdgeMask & 0b0100) && (x == resolution.x - 1)) ||
                        ((stripEdgeMask & 0b0010) && (y == 0)) ||
                        ((stripEdgeMask & 0b0001) && (y == resolution.y - 1))) {
                        markStripped(x, y);
                    }
                }
            }

            // generate the renderable data
            // finalPointsGuard.lock();
            for (int x = 0; x < resolution.x; x++) {
                for (int y = 0; y < resolution.y; y++) {
                    Point &p = points[x][y];
                    glm::vec2 &loc = points[x][y].location;

                    if (p.value > threshold && p.type == 2 && !p.stripped) {
                        addPoint(perlinCloud, loc, glm::vec3(1, 1, 0));
                    }
                    if (showCulled && p.stripped) {
                        addPoint(perlinCloud, loc, glm::vec3(1, 0, 0));
                    }
                }
            }
            // finalPointsGuard.unlock();
            perlinCloud->commit();
        }

        bool checkCollisions(glm::vec2 pointPlayer, glm::vec2 playerSize) {
            assert(object != nullptr);

            // finalPointsGuard.lock();
            for (int i = 0; i < (int)finalPoints.size(); i++) {
                bool hit = mtr::isPointInAABB2D(finalPoints[i], pointPlayer - playerSize, pointPlayer + playerSize);
                if (hit)
                    return true;
            }
            // finalPointsGuard.unlock();
            return false;
        }

        void addPoint(PointCloud *pc, glm::vec2 loc, glm::vec3 color) {
            finalPoints.emplace_back(glm::vec2(scale.x * loc.x, scale.y * loc.y));

            for (int i = 0; i < 20; i++) {
                pc->addPoint(glm::vec3(scale.x * loc.x, 0.25 * i, scale.y * loc.y), color);
            }
        }

        void markStripped(int x, int y, bool first = true) {
            // skip borders and stripped
            if (points[x][y].type != 2 || points[x][y].stripped)
                return;

            points[x][y].stripped = true;

            int has = 0;

            for (const auto &vec : findNeighbors(x, y)) {
                // std::cout << x << " " << y << " stripped" << std::endl;
                markStripped(vec.x, vec.y, false);
                has = 1;
            }

            if (first && !has) {
                std::cout << "this first has" << std::endl;
            }
        }

        // find neighbors that are above the treshold
        std::vector<glm::vec2> findNeighbors(int x, int y) {
            std::vector<glm::vec2> n;

            for (int dx = x - 1; dx <= x + 1; dx++) {
                for (int dy = y - 1; dy <= y + 1; dy++) {
                    if (dx == x && dy == x)
                        continue;
                    if (dx < 0 || dy < 0 || dx >= resolution.x || dy >= resolution.y) {
                        continue;
                    }

                    if (points[dx][dy].value > threshold) {
                        n.emplace_back(glm::vec2(dx, dy));
                    }
                }
            }
            return n;
        }

        bool findOutOfRangeNeighbors(int x, int y) {
            for (int dx = x - 1; dx <= x + 1; dx++) {
                for (int dy = y - 1; dy <= y + 1; dy++) {
                    if (dx == x && dy == x)
                        continue;
                    if (dx < 0 || dy < 0 || dx >= resolution.x || dy >= resolution.y) {
                        continue;
                    }

                    if (points[dx][dy].value <= threshold) {
                        return true;
                    }
                }
            }
            return false;
        }
    };
};
