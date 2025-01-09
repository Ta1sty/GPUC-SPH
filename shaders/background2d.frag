#version 450

layout(location = 0) in vec2 position; // position in particle space

layout(location = 0) out vec4 outColor;

layout(binding = 0) readonly buffer particleBuffer { vec2 coordinates[]; };
layout(binding = 1) uniform sampler1D colorscale;

#define GRID_SIZE 128

uint cellHash(uvec2 cell) {
    return ((cell.x * 73856093) ^ (cell.y * 19349663)) % GRID_SIZE;
}

uint getCellForCoordinate(vec2 pos) {
    return cellHash(uvec2(pos * 16));
}

float evaluateDensity(vec2 pos, float h) {
    float density = 0.0f;

    // cubic splice kernel from SPH Tutorial paper (assuming a particle mass of 1)
    for (int i = 0; i < 1024; i++) {
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
    //float value = float(getCellForCoordinate(position)) / float(GRID_SIZE);
    float value = evaluateDensity(position, 0.05) / 4;
    outColor = vec4(texture(colorscale, value).rgb, 1.0);
}
