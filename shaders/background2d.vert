#version 450

layout(push_constant) uniform PushStruct {
    mat4 mvp;
    uvec2 windowSize;
} p;

layout (location = 0) in vec2 inPos;

layout (location = 0) out vec2 position;

void main() {
    position = inPos; // position in particle space
    gl_Position = p.mvp * vec4(inPos, 0.0f, 1.0f);
}
