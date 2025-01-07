#version 450

layout(push_constant) uniform PushStruct {
    mat4 mvp;
    uvec2 windowSize;
} p;

layout (location = 0) in vec2 inPos;
//layout (location = 1) in vec3 inColor;

layout (location = 0) out vec2 outPos;

void main() {
    outPos = inPos;
    gl_Position = p.mvp * vec4(inPos, 0.0f, 1.0f);
}
