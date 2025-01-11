#pragma once

#include "utils.h"

class Camera {
public:
    glm::vec3 position = {4, 4, 4};
    float theta = glm::pi<float>() / 2, phi = glm::pi<float>() * 2.5 / 2;
    float fovy = 0.9, aspect = 1280.f / 720.f, near = 0.1, far = 100;

    Camera(Camera &other) = delete;
    Camera() {
        reset();
    }

    void reset();

    void rotateTheta(float radians);

    void rotatePhi(float radians);

    void moveInForwardDir(float distance);

    void moveInTangentDir(float distance);

    [[nodiscard]] glm::vec3 forwardDir() const;

    [[nodiscard]] glm::vec3 tangentDir() const;

    [[nodiscard]] glm::mat4 viewMatrix() const;

    [[nodiscard]] glm::mat4 projectionMatrix() const;

    [[nodiscard]] glm::mat4 viewProjectionMatrix() const;
};