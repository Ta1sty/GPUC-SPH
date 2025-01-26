#include "initialization.h"

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

using namespace glm;

using VEC_T = glm::vec3;
using UVEC_T = glm::uvec3;
using IVEC_T = glm::ivec3;
using uint = uint32_t;

using std::clamp;

#define QUANTIZATION_BOUNDS 2.0f
#define QUANTIZATION_INDEX_BITS 23
#define QUANTIZATION_CLASS_BITS 5
#define QUANTIZATION_POSITION_BITS 12

const uint indexMask = (uint(1) << QUANTIZATION_INDEX_BITS) - 1;
const uint positionMask = (uint(1) << QUANTIZATION_POSITION_BITS) - 1;
const uint classMask = (uint(1) << QUANTIZATION_CLASS_BITS) - 1;
const uint quantizationRange = positionMask;

uint dequantize_index(uint64_t data) {
    uint value = uint(data & indexMask);
    if (value == indexMask) {
        return -1;
    }
    return value;
}

uint dequantize_class(uint64_t data) {
    data = data >> QUANTIZATION_INDEX_BITS;

    uint value = uint(data | classMask);
    if (value == classMask) {
        return -1;
    }
    return value;
}


uint64_t quantize_position(VEC_T position, bool DEF_3D) {
    // [-bounds,bounds]
    VEC_T normalized = ((position / QUANTIZATION_BOUNDS) + 1.0f) * 0.5f;
    // [0,1]
    UVEC_T quanitized = UVEC_T(normalized * (float) quantizationRange);
    // [0,range]

    uint64_t value = uint64_t(0);

    if (DEF_3D) {
        value = (value | quanitized.z);
        value = value << QUANTIZATION_POSITION_BITS;
    }

    value = (value | quanitized.y);
    value = value << QUANTIZATION_POSITION_BITS;

    value = (value | quanitized.x);

    return value << (QUANTIZATION_CLASS_BITS + QUANTIZATION_INDEX_BITS);
}

VEC_T dequantize_position(uint64_t data, bool DEF_3D) {
    data = data >> (QUANTIZATION_CLASS_BITS + QUANTIZATION_INDEX_BITS);

    uint x = uint(data & positionMask);
    data = data >> QUANTIZATION_POSITION_BITS;
    uint y = uint(data & positionMask);
    data = data >> QUANTIZATION_POSITION_BITS;

    UVEC_T quantized;
    if (DEF_3D) {
        uint z = uint(data & positionMask);
        quantized = UVEC_T(x, y, z);
    } else {
        quantized = UVEC_T(x, y, 0);
    }

    // [0,qMax]
    VEC_T normalized = VEC_T(quantized) / (float) quantizationRange;
    // [0,1]
    VEC_T position = (normalized - 0.5f) * 2.0f * QUANTIZATION_BOUNDS;
    // [-bounds,bounds]

    if (!DEF_3D) position.z = 0;

    return position;
}

glm::ivec3 cellCoord(glm::vec3 position, float radius) {
    return {position / radius};
}
uint32_t cellHash(glm::ivec3 cell) {
    return ((cell.x * 73856093) ^ (cell.y * 19349663) ^ (cell.z * 83492791));
}
uint32_t cellKey(uint32_t hash, uint32_t numParticles) {
    return hash % numParticles;
}

void Simulation::check() {
    resources.device.waitIdle();

    auto cmd = spatialLookup->run(*simulationState);
    vk::SubmitInfo submit({}, {}, cmd);
    resources.computeQueue.submit(submit);

    resources.device.waitIdle();

    std::vector<float> particles(simulationParameters.numParticles * (simulationParameters.type == SceneType::SPH_BOX_2D ? 2 : 4));
    fillHostWithStagingBuffer(simulationState->particleCoordinateBuffer, particles);

    uint32_t lookupSize = nextPowerOfTwo(simulationParameters.numParticles);
    std::vector<SpatialLookupEntry> spatial_lookup(lookupSize);
    fillHostWithStagingBuffer(simulationState->spatialLookup, spatial_lookup);

    std::vector<SpatialCacheEntry> spatial_cache(lookupSize);
    fillHostWithStagingBuffer(simulationState->spatialCache, spatial_cache);

    std::vector<uint32_t> spatial_lookup_keys(lookupSize);
    for (int i = 0; i < spatial_lookup.size(); ++i) spatial_lookup_keys[i] = spatial_cache[i].cellKey;

    std::vector<SpatialIndexEntry> spatial_indices(lookupSize);
    fillHostWithStagingBuffer(simulationState->spatialIndices, spatial_indices);

    std::vector<SpatialCacheEntry> spatial_lookup_sorted(spatial_cache.begin(), spatial_cache.end());
    std::sort(spatial_lookup_sorted.begin(), spatial_lookup_sorted.end(),
              [](SpatialCacheEntry left, SpatialCacheEntry right) -> bool {
                  if (left.cellKey == right.cellKey)
                      return left.cellClass < right.cellClass;
                  return left.cellKey < right.cellKey;
              });

    std::set<uint32_t> keys;
    std::vector<SpatialHashResult> hashes;

    auto dimensions = simulationParameters.type == SceneType::SPH_BOX_3D ? 3 : 2;
    for (uint32_t i = 0; i < lookupSize; i++) {
        SpatialLookupEntry lookup = spatial_lookup[i];
        SpatialCacheEntry cache = spatial_cache[i];

        keys.insert(cache.cellKey);

        uint32_t particleIndex = dequantize_index(lookup.data);

        // check index consistency
        if (cache.cellKey == -1) {
            if (particleIndex != -1) {
                throw std::runtime_error("cache indicates invalid particle, but particle index is not -1");
            }
        } else {
            if (particleIndex == -1) {
                throw std::runtime_error("cache indicates valid particle, but particle index is -1");
            }
        }

        bool def_3d = simulationParameters.type == SceneType::SPH_BOX_3D;

        auto dequantizedPosition = dequantize_position(lookup.data, def_3d);

        glm::vec3 position;
        switch (simulationParameters.type) {
            case SceneType::SPH_BOX_2D:
                position = glm::vec3(particles[particleIndex * 2], particles[particleIndex * 2 + 1], 0);
                break;
            case SceneType::SPH_BOX_3D:
                position = glm::vec3(particles[particleIndex * 4], particles[particleIndex * 4 + 1], particles[particleIndex * 4 + 2]);
                break;
        }

        for (int d = 0; d < dimensions; d++) {
            auto difference = std::abs(dequantizedPosition[d] - position[d]);
            if (difference > 0.002) {
                throw std::runtime_error("quantization is to different from the actual position");
            }
        }

        uint64_t reQuantizedPosition = quantize_position(position, def_3d);
        glm::vec3 correctedPosition = dequantize_position(reQuantizedPosition, def_3d);

        glm::ivec3 cell = cellCoord(correctedPosition, simulationState->spatialRadius);
        uint32_t testKey = cellKey(cellHash(cell), simulationParameters.numParticles);

        SpatialHashResult result {
                cache.cellKey,
                testKey,
                position.x,
                position.y,
                position.z,
                cell.x,
                cell.y,
                cell.z,
        };

        keys.emplace(cache.cellKey);
        hashes.emplace_back(result);

        if (result.lookupKey != result.testKey) {
            throw std::runtime_error("key differs");
        }

        if (spatial_cache[i].cellKey != spatial_lookup_sorted[i].cellKey || spatial_cache[i].cellClass != spatial_lookup_sorted[i].cellClass) {
            throw std::runtime_error("spatial lookup not sorted");
        }
    }

    std::map<uint32_t, std::vector<SpatialHashResult>> groupedResults;

    for (const auto &hash: hashes) {
        uint32_t lookupKey = hash.lookupKey;

        auto &group = groupedResults[lookupKey];
        bool isDistinct = true;

        for (const auto &element: group) {
            if (element.cellX == hash.cellX && element.cellY == hash.cellY && element.cellZ == hash.cellZ) {
                isDistinct = false;
                break;
            }
        }

        if (isDistinct) {
            group.push_back(hash);
        }
    }

    std::map<uint32_t, std::vector<SpatialHashResult>> collisions;

    for (const auto &[lookupKey, group]: groupedResults) {
        if (group.size() >= 2) {
            collisions[lookupKey] = group;
        }
    }

    uint32_t collisionCellCount = 0;
    for (const auto &item: collisions) collisionCellCount += item.second.size();

    std::cout << "Hash-Collisions "
              << "Key-Count: " << keys.size() << " "
              << "Collision-Count: " << collisions.size() << " "
              << "Cell-Count: " << collisionCellCount << " "
              << std::endl;
    int a = 0;
}