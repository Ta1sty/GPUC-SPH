#include "camera.h"
#include "glm/ext/matrix_clip_space.hpp"

void Camera::reset() {
    position = glm::vec3(0.5, 2, 0.9);
    phi = glm::pi<float>();
    theta = 0.4 * glm::pi<float>();
    aspect = (float) resources.extent.width / (float) resources.extent.height;
}

glm::vec3 Camera::forwardDir() const {
    return {
            std::sin(phi) * std::sin(theta),
            std::cos(phi) * std::sin(theta),
            -std::cos(theta)};
}

glm::vec3 Camera::tangentDir() const {
    return {
            std::cos(phi),
            -std::sin(phi),
            0.f};
}

void Camera::rotateTheta(float radians) {
    theta = std::clamp(theta + radians, 0.f, glm::pi<float>());
}

void Camera::rotatePhi(float radians) {
    phi += radians;
    phi = std::fmod(phi, 2 * glm::pi<float>());
    if (phi < 0.f)
        phi += 2 * glm::pi<float>();
}

void Camera::moveInForwardDir(float distance) {
    position += forwardDir() * distance;
}

void Camera::moveInTangentDir(float distance) {
    position += tangentDir() * distance;
}

void Camera::moveInUpDir(float distance) {
    position.z += distance;
}

glm::mat4 Camera::viewMatrix() const {
    auto translationMatrix = glm::translate(glm::mat4(1.0), -position);
    auto rotationMatrixTheta = glm::rotate(glm::mat4(1.0), -theta, glm::vec3(1, 0, 0));
    auto rotationMatrixPhi = glm::rotate(glm::mat4(1.0), phi, glm::vec3(0, 0, 1));
    return rotationMatrixTheta * rotationMatrixPhi * translationMatrix;
}

glm::mat4 Camera::projectionMatrix() const {
    return glm::perspective(fovy, aspect, near, far);
}

glm::mat4 Camera::viewProjectionMatrix() const {
    return projectionMatrix() * viewMatrix();
}
