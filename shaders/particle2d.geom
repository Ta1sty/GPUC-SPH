#version 450

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

//layout (binding = 0) readonly buffer ParticlePositions { vec2 gParticlePositions[]; }

//layout (location = 0) vec3 particleColor;
layout (location = 0) out vec2 particleRelativePosition;

#define VERTEX(x, y) \
    gl_Position = centerPosition + vec4(x, y, 0, 0) * scaleFactor; \
    particleRelativePosition = vec2(x, y); \
    EmitVertex();

void main() {
    float scaleFactor = 0.02;
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
