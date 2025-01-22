#pragma once

#include "initialization.h"
#include "simulation_state.h"


struct SpatialHashResult {
    uint32_t lookupKey;
    uint32_t testKey;
    float x;
    float y;
    float z;
    int32_t cellX;
    int32_t cellY;
    int32_t cellZ;
};

struct ParticleNeighbour {
    float distance;
    uint32_t index;
    uint32_t cellKey;
};

glm::ivec3 cellCoord(glm::vec3 position, float radius);
uint32_t cellHash(glm::ivec3 cell);
uint32_t cellKey(uint32_t hash, uint32_t numParticles);

void getNeighbours(
        glm::vec2 pos,
        float spatialRadius,
        uint32_t numParticles,
        std::vector<SpatialLookupEntry> spatial_lookup,
        std::vector<uint32_t> spatial_indices,
        std::vector<glm::vec2> coordinates,
        std::vector<ParticleNeighbour> neighbours);