#version 450

layout(location = 0) in vec2 position; // position in particle space

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler1D colorscale;

#define GRID_SIZE 128

uint cellHash(uvec2 cell) {
    return ((cell.x * 73856093) ^ (cell.y * 19349663)) % GRID_SIZE;
}

void main() {
    uint cell = cellHash(uvec2(position * 16));
    outColor = vec4(texture(colorscale, float(cell) / float(GRID_SIZE)).rgb, 1.0);
}
