#version 450

#include "_defines.glsl"

layout (location = 0) in vec2 particleRelativePosition;
layout (location = 1) in vec3 particleCenter;

layout (location = 0) out vec4 outColor;

layout (binding = 0) readonly buffer particleBuffer { VEC_T coordinates[]; };
layout (binding = 5) readonly buffer velocityBuffer { VEC_T velocities[]; };
layout (binding = 6) readonly buffer densityBuffer { float densities[]; };
layout (binding = 1) uniform sampler1D colorscale;

layout (binding = 2) uniform UniformBuffer {
    uint numParticles;
    uint backgroundField;
    uint particleColor;
    float particleRadius;
    float spatialRadius;
};

layout(push_constant) uniform PushStruct {
    mat4 mvp;
    uvec2 windowSize;
    float targetDensity;
} p;

#define GRID_BINDING_LOOKUP 3
#define GRID_BINDING_INDEX 4
#define GRID_BINDING_COORDINATES -1
#define COORDINATES_BUFFER_NAME coordinates
#define GRID_NUM_ELEMENTS numParticles
#define GRID_CELL_SIZE spatialRadius
#include "spatial_lookup.glsl"

// https://thebookofshaders.com/07/
float circle(in vec2 dist, in float _radius) {
    return 1.0 - smoothstep(_radius - (_radius / particleRadius * 4), // these radius bounds are arbitrary
                            _radius + (_radius / particleRadius * 4), // picked by what seems to look good
                            dot(dist, dist));
}

uint countNeighbours(VEC_T position) {
    uint count = 0;

    FOREACH_NEIGHBOUR(position, count += 1);

    return count;
}

float neighbourCountNormalized(VEC_T position) {
    float count = float(countNeighbours(position));
    return 1.0f - exp((1.0f - count) / 16.0f); // normalize to [0,1) with some function that is totally not made up
}

void main() {
    //        uint cell = cellHash(uvec2(particleCenter * 8));
    //        outColor = vec4(texture(colorscale, float(cell) / float(GRID_BUFFER_SIZE)).rgb, 1.0);
    #ifdef DEF_2D
    VEC_T center = particleCenter.xy;
    #else
    VEC_T center = particleCenter;
    #endif

    vec3 color = vec3(1.0f);
    switch (particleColor) {
        case 2: // num_neighbours
            color = texture(colorscale, neighbourCountNormalized(center)).rgb;
            break;
        case 3: // density
            color = texture(colorscale, min(densities[gl_PrimitiveID] / (2.0f * p.targetDensity), 1.0)).rgb;
            break;
        case 4: // velocity
            color = texture(colorscale, length(velocities[gl_PrimitiveID])).rgb;
            break;
    }

    float alpha = circle(particleRelativePosition, 0.90f);
    outColor = vec4(color * circle(particleRelativePosition, 0.5f), alpha);
    if (alpha <= 0.2f)
        gl_FragDepth = 1.0f;
    else
        gl_FragDepth = gl_FragCoord.z;
}
