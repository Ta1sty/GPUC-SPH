#version 450

layout (location = 0) out vec4 outColor;
#ifndef DEF_2D // this definitely won't work in 2d .-.

#define EPSILON 0.0001
#include "_defines.glsl"

layout (push_constant) uniform PushStruct {
    mat4 mvp;
    vec3 cameraPos;
    vec2 nearFar;
    float targetDensity;
} p;

layout (location = 0) in vec3 worldPosition;

layout (binding = 0) uniform UniformBuffer {
    uint numParticles;
    uint backgroundField;
    uint particleColor;
    float particleRadius;
    float spatialRadius;
};
layout (input_attachment_index = 0, binding = 1) uniform subpassInput depthImage;
layout (binding = 2) uniform sampler1D colorscale;
layout (binding = 3) uniform sampler3D densityVolume;

const float STEP_SIZE = 0.001;

bool isInVolume(vec3 position) {
    return position.x >= 0.0f && position.x <= 1.0f
    && position.y >= 0.0f && position.y <= 1.0f
    && position.z >= 0.0f && position.z <= 1.0f;
}

/**
 * Decides the volume coefficients from our simulation data.
 */
void sampleVolume(in vec3 position, out float extinction, out vec3 emission) {
    float density;
    switch (backgroundField) {
        case 3:
            density = clamp(texture(densityVolume, position).r / (2.0f * p.targetDensity), 0, 1);
            break;
        default:
            density = 0.0f;
    }

    float diff = abs(density - 0.5);
    extinction = sqrt(density) * 2 + 20 * exp(-2000 * diff * diff);
    emission = texture(colorscale, sqrt(density)).rgb;
}

void main() {
    float fragDepth = subpassLoad(depthImage).r;
    // I'm not really sure if this is correct
    float maxT = fragDepth * (p.nearFar.y - p.nearFar.x);

    vec3 direction = worldPosition - p.cameraPos;
    float t = length(direction);
    direction /= t; // normalize with precomputed length, t is needed for check against depth

    float transmittance = 1.0f;
    vec3 color = vec3(0.0f);

    vec3 x = worldPosition;
    float stepSize = STEP_SIZE;
    do {
        t += stepSize;
        if (t >= maxT) {
            stepSize += maxT - t;
        }

        x += stepSize * direction;

        float localExtinction;
        vec3 emissionColor;
        sampleVolume(x, localExtinction, emissionColor);
        float stepTransmittance = exp(-localExtinction * stepSize);
        transmittance *= stepTransmittance;
        vec3 emission = emissionColor * (1 - stepTransmittance);

        color += transmittance * emission;
    } while (t < maxT && isInVolume(x)); // exiting segment will not get included, not a fun fix

    outColor = vec4(color, 1 - transmittance);
}

#else
void main() {
    outColor = vec4(1.0f, 0.0, 0.0, 1.0f);
}
#endif
