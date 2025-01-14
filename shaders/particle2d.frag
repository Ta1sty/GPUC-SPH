#version 450

layout (location = 0) in vec2 particleRelativePosition;
layout (location = 1) in vec2 particleCenter;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler1D colorscale;

layout (binding = 2) uniform UniformBuffer {
    uint numParticles;
    uint backgroundField;
    float particleRadius;
};


#define GRID_BUFFER_SIZE 128

uint cellHash(uvec2 cell) {
    return ((cell.x * 73856093) ^ (cell.y * 19349663)) % GRID_BUFFER_SIZE;
}

// https://thebookofshaders.com/07/
float circle(in vec2 dist, in float _radius) {
    return 1.0 - smoothstep(_radius - (_radius / particleRadius * 4), // these radius bounds are arbitrary
                            _radius + (_radius / particleRadius * 4), // picked by what seems to look good
                            dot(dist, dist));
}

void main() {
    //        uint cell = cellHash(uvec2(particleCenter * 8));
    //        outColor = vec4(texture(colorscale, float(cell) / float(GRID_BUFFER_SIZE)).rgb, 1.0);
    vec3 color = vec3(circle(particleRelativePosition, 0.5f));
    outColor = vec4(color, circle(particleRelativePosition, 0.90f));
}
