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
layout (binding = 4) uniform sampler2D environment;

const float STEP_SIZE = 0.001;

bool isInVolume(vec3 position) {
    return position.x >= 0.0f && position.x <= 1.0f
    && position.y >= 0.0f && position.y <= 1.0f
    && position.z >= 0.0f && position.z <= 1.0f;
}

float sampleDensity(in vec3 position) {
    return clamp(texture(densityVolume, position).r / (2.0f * p.targetDensity), 0, 1);
}

/**
 * Decides the volume coefficients from our simulation data.
 */
bool sampleVolume(in vec3 position, out float extinction, out vec3 emission) {
    float density;
    switch (backgroundField) {
        case 3:
            density = sampleDensity(position);
            break;
        default:
            density = 0.0f;
    }

//    float diff = abs(density - 0.5);
//    extinction = sqrt(density) * 2 + 20 * exp(-2000 * diff * diff);
//    emission = texture(colorscale, sqrt(density)).rgb;
    extinction = (density > 0.4) ? 0.1f : 0.0f;
    emission = vec3(0.84, 0.91, 0.41);

    return density > 0.4;
}

vec3 sampleEnvironment(vec3 direction) {
    const float PI = 3.141592653589793;
    const float TWO_PI = 6.283185307179586;
    float yaw = atan(direction.y, direction.x);
    float pitch = asin(direction.z);
    vec2 uv = vec2((yaw / TWO_PI) + 0.5, 0.5 - pitch / PI);
    return texture(environment, uv).rgb;
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

    bool isInMedium = false;

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
        bool _isInMedium = sampleVolume(x, localExtinction, emissionColor);
        if (_isInMedium != isInMedium) {// refraction on surface transition
            const float delta = 1.0f / 800.0f;
            float n = 1.3325; // https://refractiveindex.info/?shelf=3d&book=liquids&page=water
            vec3 gradient = vec3(
                sampleDensity(x + vec3(delta, 0.0f, 0.0f)) - sampleDensity(x - vec3(delta, 0.0f, 0.0f)),
                sampleDensity(x + vec3(0.0f, delta, 0.0f)) - sampleDensity(x - vec3(0.0f, delta, 0.0f)),
                sampleDensity(x + vec3(0.0f, 0.0f, delta)) - sampleDensity(x - vec3(0.0f, 0.0f, delta))
            );
            vec3 normal = normalize(gradient);
            if (!isInMedium) {
                normal *= -1;
                n = 1.0f / n;
            }

            vec3 refracted = refract(-direction, normal, n);
            vec3 reflected = reflect(-direction, normal);
            if (refracted == vec3(0.0)) {
                refracted = direction; // whatever
            }
            refracted = normalize(refracted);

            const float R_0 = pow((n - 1) / (n + 1), 2.0f);
            const float R = R_0 + (1 - R_0) * (1 - dot(-direction, normal));
            transmittance *= (1 - R);
            color += R * sampleEnvironment(reflected);

            direction = refracted;
            isInMedium = _isInMedium;
        }

        float stepTransmittance = exp(-localExtinction * stepSize);
        transmittance *= stepTransmittance;
        vec3 emission = emissionColor * (1 - stepTransmittance);

        color += transmittance * emission;
    } while (t < maxT && isInVolume(x)); // exiting segment will not get included, not a fun fix

    vec3 backgroundColor = sampleEnvironment(direction);
    outColor = vec4(transmittance * backgroundColor + (1 - transmittance) * color, 1.0f);
}

#else
void main() {
    outColor = vec4(1.0f, 0.0, 0.0, 1.0f);
}
#endif
