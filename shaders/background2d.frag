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

float evaluateDensity(vec2 pos, float h) {
    float density = 0.0f;

    // cubic splice kernel from SPH Tutorial paper (assuming a particle mass of 1)
    for (int i = 0; i < numParticles; i++) {
        float distance = length(coordinates[i] - pos);
        float q = distance / h;
        if (q <= 0.5) {
            density += 6 * (q * q * q - q * q) + 1;
        } else if (q <= 1) {
            float _q = 1 - q;
            density += 2 * _q * _q * _q;
        }
    }

    return density;
}

void main() {
    //float value = float(getCellForCoordinate(position)) / float(GRID_BUFFER_SIZE);
    float value = 0.0f;
    switch (backgroundField) {
        case 0:
            outColor = cellColor(cellKey(position));
            break;
        case 1:
            value = evaluateDensity(position, 0.05) / 4;
            outColor = vec4(texture(colorscale, value).rgb, 1.0);
            break;
        default:
            outColor = vec4(texture(colorscale, value).rgb, 1.0);
            break;
    }
}
