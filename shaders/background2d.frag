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
#define GRID_NUM_ELEMENTS numParticles
#define GRID_CELL_SIZE spatialRadius
#include "spatial_lookup.glsl"

void addDensity(inout float density, uint neighbourIndex, vec2 neighbourPosition, float neighbourDinstance) {
    float normalized = neighbourDinstance / spatialRadius;
    density += 1 - normalized;
}

float evaluateDensity(vec2 position) {
    float density = 0.0f;

    FOREACH_NEIGHBOUR(position, addDensity(density, NEIGHBOUR_INDEX, NEIGHBOUR_POSITION, NEIGHBOUR_DISTANCE));

    return min(density / (numParticles * spatialRadius * spatialRadius * 2), 1);
}

void main() {
    //float value = float(getCellForCoordinate(position)) / float(GRID_NUM_ELEMENTS);
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
