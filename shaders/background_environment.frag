#version 450

#define PI 3.141592653589793
#define TWO_PI 6.283185307179586

layout (location = 0) in vec2 quadUV;
layout (location = 1) in vec3 inDirection;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D environment;

void main() {
    vec3 direction = normalize(inDirection);

    float yaw = atan(direction.y, direction.x);
    float pitch = asin(direction.z);

    vec2 uv = vec2((yaw / TWO_PI) + 0.5, 0.5 - pitch / PI);

    outColor = texture(environment, uv);
}
