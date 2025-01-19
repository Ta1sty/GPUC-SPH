#pragma once

#include "initialization.h"
#include "simulation_state.h"


struct SpatialHashResult {
    uint32_t lookupKey;
    uint32_t testKey;
    float x;
    float y;
    uint32_t cellX;
    uint32_t cellY;
};

struct ParticleNeighbour {
    float distance;
    uint32_t index;
    uint32_t cellKey;
};

glm::uvec2 getCell(glm::vec2 position, float spatialRadius);
uint32_t getCellKey(glm::vec2 position, float spatialRadius, uint32_t numParticles);

void getNeighbours(
        glm::vec2 pos,
        float spatialRadius,
        uint32_t numParticles,
        std::vector<SpatialLookupEntry> spatial_lookup,
        std::vector<uint32_t> spatial_indices,
        std::vector<glm::vec2> coordinates,
        std::vector<ParticleNeighbour> neighbours);