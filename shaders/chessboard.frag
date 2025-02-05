#version 450

layout (location = 0) in vec2 position;

layout (location = 0) out vec4 outColor;

void main() {
    float l = float(int(position.x * 8) % 2 ^ int(position.y * 8) % 2);
    outColor = vec4(l, l, l, 1.0f);
}
