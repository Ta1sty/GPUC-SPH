#version 450

layout(push_constant) uniform PushStruct {
    mat4 mvp;
    uvec2 windowSize;
    float targetDensity;
} p;

layout (location = 0) in vec3 inPos;
//layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 outPos;

void main() {
    outPos = inPos;
    gl_Position = p.mvp * vec4(inPos, 1.0f);
}
