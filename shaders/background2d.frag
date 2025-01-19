#version 450

layout (location = 0) in vec2 position; // position in particle space

layout (location = 0) out vec4 outColor;

layout (binding = 0) readonly buffer particleBuffer { vec2 coordinates[]; };
layout (binding = 1) uniform sampler1D colorscale;
layout (binding = 2) uniform UniformBuffer {
    uint numParticles;
    uint backgroundField;
    float particleRadius;
    float spatialRadius;
};

#define GRID_BINDING_LOOKUP 3
#define GRID_BINDING_INDEX 4
#define GRID_BUFFER_SIZE numParticles
#define GRID_CELL_SIZE spatialRadius
#include "spatial_lookup.glsl"

#define OFFSET_COUNT 9
const vec2 offsets[OFFSET_COUNT] = {
vec2(-1, -1),
vec2(-1, 0),
vec2(-1, 1),
vec2(0, -1),
vec2(0, 0),
vec2(0, 1),
vec2(1, -1),
vec2(1, 0),
vec2(1, 1),
};

float evaluateDensity(vec2 pos) {
    float density = 0.0f;

    for (int i = 0; i < OFFSET_COUNT; i++) {
        vec2 offset = vec2(offsets[i].x * GRID_CELL_SIZE, offsets[i].y * GRID_CELL_SIZE);
        uint cellKey = cellKey(pos + offset);
        uint index = spatial_indices[cellKey];

        while (index < numParticles) {
            SpatialLookupEntry entry = spatial_lookup[index];
            index++;

            // different hash
            if (entry.cellKey != cellKey) {
                break;
            }
            vec2 position = coordinates[entry.particleIndex];

            // cubic splice kernel from SPH Tutorial paper (assuming a particle mass of 1)
            float distance = length(position - pos);

            if (distance > spatialRadius) continue;

            float normalized = distance / spatialRadius;
            density += 1 - normalized;

            //            if (q <= 0.5) {
            //                density += 6 * (q * q * q - q * q) + 1;
            //            } else if (q <= 1) {
            //                float _q = 1 - q;
            //                density += 2 * _q * _q * _q;
            //            }

        }
    }

    return min(density / (numParticles * spatialRadius * spatialRadius * 2), 1);
}

void main() {
    //float value = float(getCellForCoordinate(position)) / float(GRID_BUFFER_SIZE);
    float value = 0.0f;
    switch (backgroundField) {
        case 0:
            outColor = cellColor(cellKey(position));
            break;
        case 1:
            value = evaluateDensity(position);
            outColor = vec4(texture(colorscale, value).rgb, 1.0);
            break;
        default:
            outColor = vec4(texture(colorscale, value).rgb, 1.0);
            break;
    }
}
