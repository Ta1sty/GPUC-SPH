#version 450

layout(location = 0) in vec2 particleRelativePosition;
layout(location = 1) in vec2 particleCenter;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler1D colorscale;

#define GRID_SIZE 128

uint cellHash(uvec2 cell) {
    return ((cell.x * 73856093) ^ (cell.y * 19349663)) % GRID_SIZE;
}

void main() {
    if (dot(particleRelativePosition, particleRelativePosition) < 1.0f) {
        uint cell = cellHash(uvec2(particleCenter * 8));
        outColor = vec4(texture(colorscale, float(cell) / float(GRID_SIZE)).rgb, 1.0);
    }
}
