#include "debug.h"


glm::ivec3 cellCoord(glm::vec3 position, float radius) {
    return {position / radius};
}
uint32_t cellHash(glm::ivec3 cell) {
    return ((cell.x * 73856093) ^ (cell.y * 19349663) ^ (cell.z * 83492791));
}
uint32_t cellKey(uint32_t hash, uint32_t numParticles) {
    return hash % numParticles;
}

#define OFFSET_COUNT 9
glm::vec2 offsets[OFFSET_COUNT] = {
        glm::vec2(-1, -1),
        glm::vec2(-1, 0),
        glm::vec2(-1, 1),
        glm::vec2(0, -1),
        glm::vec2(0, 0),
        glm::vec2(0, 1),
        glm::vec2(1, -1),
        glm::vec2(1, 0),
        glm::vec2(1, 1),
};