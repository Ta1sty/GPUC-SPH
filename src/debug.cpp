#include "debug.h"


glm::ivec2 cellCoord(glm::vec2 position, float radius) {
    glm::ivec2 cell(int(position.x / radius), int(position.y / radius));
    return cell;
}
uint32_t cellHash(glm::ivec2 cell) {
    uint32_t key = ((cell.x * 73856093) ^ (cell.y * 19349663));
    return key;
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