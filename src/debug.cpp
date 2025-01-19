#include "debug.h"

glm::uvec2 getCell(glm::vec2 position, float spatialRadius) {
    float inverseSize = 1.f / spatialRadius;

    glm::vec2 scaled(position.x * inverseSize, position.y * inverseSize);
    glm::uvec2 cell(scaled.x, scaled.y);
    return cell;
}

uint32_t getCellKey(glm::vec2 position, float spatialRadius, uint32_t numParticles) {
    float inverseSize = 1.f / spatialRadius;

    glm::vec2 scaled(position.x * inverseSize, position.y * inverseSize);
    glm::uvec2 cell(scaled.x, scaled.y);

    uint32_t hash = ((cell.x * 73856093) ^ (cell.y * 19349663));
    uint32_t cellKey = hash % numParticles;
    return cellKey;
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

void getNeighbours(glm::vec2 pos, float spatialRadius, uint32_t numParticles, std::vector<SpatialLookupEntry> spatial_lookup, std::vector<uint32_t> spatial_indices, std::vector<glm::vec2> coordinates, std::vector<ParticleNeighbour> neighbours) {

    neighbours.clear();

    for (int i = 0; i < OFFSET_COUNT; i++) {
        uint32_t cellKey = getCellKey(pos + spatialRadius * offsets[i], spatialRadius, numParticles);
        uint32_t index = spatial_indices[cellKey];

        while (index < numParticles) {
            SpatialLookupEntry entry = spatial_lookup[index];

            // different hash
            if (entry.cellKey != cellKey) {
                break;
            }
            glm::vec2 position = coordinates[entry.particleIndex];

            ParticleNeighbour neighbour {
                    .distance = glm::length(position - pos),
                    .index = entry.particleIndex,
                    .cellKey = entry.cellKey,
            };

            neighbours.emplace_back(neighbour);

            index++;
        }
    }
}
