#version 450

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(push_constant) uniform PushStruct {
    uvec2 windowSize;
} p;

//layout (binding = 0) readonly buffer ParticlePositions { vec2 gParticlePositions[]; }

//layout (location = 0) vec3 particleColor;
layout (location = 0) out vec2 particleRelativePosition;

#define VERTEX(x, y) \
    gl_Position = centerPosition + vec4(vec2(x, y) * scaleFactor, 0.0f, 0.0f); \
    particleRelativePosition = vec2(x, y); \
    EmitVertex();

void main() {
    vec2 scaleFactor = vec2(
        5.0f / float(p.windowSize.x),
        5.0f / float(p.windowSize.y)
    );
    vec4 centerPosition = gl_in[0].gl_Position;


    // C         D
    // *-------- *
    // | \       |
    // |   \     |
    // |     \   |
    // |       \ |
    // *---------*
    // A          B

    VERTEX(-1,  1); // A
    VERTEX( 1,  1); // B
    VERTEX(-1, -1); // C
    VERTEX( 1, -1); // D
    EndPrimitive();
}
