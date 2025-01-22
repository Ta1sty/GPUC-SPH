#version 450

layout(push_constant) uniform PushStruct {
    mat4 mvp;
    vec3 cameraPos;
} p;

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec3 outWorldPosition;

void main() {
    vec3 worldPos = (inPos + vec3(1.0f)) / 2.0f;

    outWorldPosition = worldPos;
    gl_Position = p.mvp * vec4(worldPos, 1.0f);
}
