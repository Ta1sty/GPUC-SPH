#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;

const vec3 lightPos = vec3(0.5, 0.5, 0.7);
const vec3 lightIntensity = vec3(0.05);

void main() {
    vec3 toLight = lightPos - pos;
    float distSq = dot(toLight, toLight);
    toLight /= sqrt(distSq);
    // todo face normals
    // todo the vertex sormals of cubejump seem to get flipped when loading
    outColor = vec4(albedo * abs(dot(normalize(normal), toLight)) / distSq * lightIntensity, 1);
}