#version 450

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (location = 0) in vec3 inParticleCoordinate[];

layout(push_constant) uniform PushStruct {
    mat4 mvp;
    uvec2 windowSize;
    float targetDensity;
} p;

layout(binding = 2) uniform UniformBuffer {
    uint numParticles;
    uint backgroundField;
    uint particleColor;
    float particleRadius;
    float spatialRadius;
};

//layout (binding = 0) readonly buffer ParticlePositions { vec2 gParticlePositions[]; }

//layout (location = 0) vec3 particleColor;
layout (location = 0) out vec2 particleRelativePosition;
layout (location = 1) out vec3 outParticleCoordinate;

#define VERTEX(x, y) \
    gl_Position = centerPosition + vec4(vec2(x, y) * scaleFactor, 0.0f, 0.0f); \
    particleRelativePosition = vec2(x, y); \
    outParticleCoordinate = inParticleCoordinate[0]; \
    gl_PrimitiveID = gl_PrimitiveIDIn; \
    EmitVertex()

void main() {
    vec2 scaleFactor = vec2(
        particleRadius / float(p.windowSize.x),
        particleRadius / float(p.windowSize.y)
    );
    vec4 centerPosition = gl_in[0].gl_Position;

    VERTEX(-1,  1); // A
    VERTEX(-1, -1); // C
    VERTEX( 1,  1); // B
    VERTEX( 1, -1); // D
    EndPrimitive();
}
