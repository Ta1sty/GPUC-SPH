#version 450

#include "_defines.glsl"

layout (binding = 0) readonly buffer particleBuffer { VEC_T coordinates[]; };
layout (binding = 1) uniform sampler1D colorscale;
layout (binding = 2) uniform UniformBuffer {
    uint numParticles;
    uint backgroundField;
    uint particleColor;
    float particleRadius;
    float spatialRadius;
};

#define GRID_BINDING_LOOKUP 3
#define GRID_BINDING_INDEX 4
#define GRID_NUM_ELEMENTS numParticles
#define GRID_CELL_SIZE spatialRadius
#define COORDINATES_BUFFER_NAME coordinates
#include "spatial_lookup.glsl"

layout (location = 0) in vec2 position;

layout (location = 0) out vec4 outColor;

void addDensity(inout float density, uint neighbourIndex, VEC_T neighbourPosition, float neighbourDinstance) {
    float normalized = neighbourDinstance / spatialRadius;
    density += 1 - normalized;
}

float evaluateDensity(VEC_T position) {
    float density = 0.0f;

    FOREACH_NEIGHBOUR(position, addDensity(density, NEIGHBOUR_INDEX, NEIGHBOUR_POSITION, NEIGHBOUR_DISTANCE));

    return min(density / (numParticles * spatialRadius * spatialRadius * 2), 1);
}

void main() {
    //float value = float(getCellForCoordinate(position)) / float(GRID_NUM_ELEMENTS);
    float value = 0.0f;
    VEC_T position_t = SWIZZLE(vec3(position, 0));
    switch (backgroundField) {
        case 1:
            outColor = cellColor(cellKey(position_t));
            break;
        case 2:
            value = evaluateDensity(position_t);
            outColor = vec4(texture(colorscale, value).rgb, 1.0);
            break;
        default:
            outColor = vec4(1.0f, 0.0f, 0.0f, 1.0);
            break;
    }
}
