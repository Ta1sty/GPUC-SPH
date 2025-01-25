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

struct ParticleNeighbour {
    float distance;
    uint32_t index;
    uint32_t cellKey;
};

using namespace glm;

using VEC_T = glm::vec3;
using UVEC_T = glm::uvec3;
using IVEC_T = glm::ivec3;
using uint = uint32_t;

using std::clamp;

#define SWIZZLE(x) (x)


#define QUANTIZATION_BOUNDS 8.0f
#define QUANTIZATION_INDEX_BITS 25
#define QUANTIZATION_POSITION_BITS 13

const uint indexMask = (uint(1) << QUANTIZATION_INDEX_BITS) - 1;
const uint positionMask = (uint(1) << QUANTIZATION_POSITION_BITS) - 1;
const uint quantizationRange = positionMask;

uint64_t quantize_index(uint index) {
    if (index == -1) {
        return indexMask;
    }
    return (index & indexMask);
}

uint64_t quantize_position(VEC_T position) {
    // [-bounds,bounds]
    VEC_T normalized = ((position / QUANTIZATION_BOUNDS) + 1.0f) * 0.5f;
    // [0,1]
    UVEC_T quanitized = UVEC_T(normalized * (float) quantizationRange);
    // [0,range]

    uint64_t value = uint64_t(0);

    value = (value | quanitized.z);
    value = value << QUANTIZATION_POSITION_BITS;

    value = (value | quanitized.y);
    value = value << QUANTIZATION_POSITION_BITS;

    value = (value | quanitized.x);
    value = value << QUANTIZATION_INDEX_BITS;

    return value;
}

uint64_t quanitize(uint index, VEC_T position) {
    return quantize_position(position) | quantize_index(index);
}

uint dequantize_index(uint64_t data) {
    uint value = uint(data & indexMask);
    if (value == indexMask) {
        return -1;
    }
    return value;
}

VEC_T dequantize_position(uint64_t data) {
    data = data >> QUANTIZATION_INDEX_BITS;
    uint x = uint(data & positionMask);
    data = data >> QUANTIZATION_POSITION_BITS;
    uint y = uint(data & positionMask);
    data = data >> QUANTIZATION_POSITION_BITS;
    uint z = uint(data & positionMask);
    UVEC_T quantized = UVEC_T(x, y, z);

    // [0,qMax]
    VEC_T normalized = VEC_T(quantized) / (float) quantizationRange;
    // [0,1]
    VEC_T position = (normalized - 0.5f) * 2.0f * QUANTIZATION_BOUNDS;
    // [-bounds,bounds]
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