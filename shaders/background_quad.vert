#version 450

layout(push_constant) uniform PushStruct {
    mat4 mvpInv;
    vec3 cameraPos;
} p;

layout (location = 0) in vec2 inPos;
layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outDirection;

void main() {
    vec2 cameraSpace = 2 * inPos - vec2(1.0f);
    gl_Position = vec4(cameraSpace, 1.0f, 1.0f);
    outUV = vec2(inPos.x, 1 - inPos.y);

    vec4 tmp = p.mvpInv * vec4(cameraSpace, 0.0f, 1.0f);
    outDirection = (tmp.xyz / tmp.w) - p.cameraPos;
}
