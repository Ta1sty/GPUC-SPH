#version 450

layout (location = 0) in vec2 inPos;
//layout (location = 1) in vec3 inColor;

//layout (location = 0) vec3 outColor;

void main() {
    gl_Position = vec4(2 * inPos - vec2(1.0), 0.0f, 1.0f);
    // outColor = inColor;
}
