#version 450

layout (location = 0) in vec3 worldPosition;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4((worldPosition + vec3(1.0f)) / 2.0f, 0.5f);
}
