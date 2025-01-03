#version 450

layout(location = 0) in vec2 particleRelativePosition;

layout(location = 0) out vec4 outColor;

void main() {
    if (dot(particleRelativePosition, particleRelativePosition) < 1.0f)
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
