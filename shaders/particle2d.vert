#version 450

layout(push_constant) uniform PushStruct {
    uvec2 windowSize;
} p;

layout (location = 0) in vec2 inPos;
//layout (location = 1) in vec3 inColor;

//layout (location = 0) vec3 outColor;

void main() {
    float windowAspectRatio = float(p.windowSize.x) / float(p.windowSize.y);
    vec2 pos = 2 * inPos - vec2(1.0);

    if (windowAspectRatio > 1.0)
        pos.x /= windowAspectRatio;
    else
        pos.y *= windowAspectRatio;


    gl_Position = vec4(pos, 0.0f, 1.0f);
    // outColor = inColor;
}
