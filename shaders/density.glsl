const float PI = 3.14159265359;
const float particleMass = 1.0;

float smoothingKernel(float radius, float dist) {
    if (dist >= radius) return 0.0;
    #ifdef DEF_2D
    float volume = (PI * pow(radius, 4)) / 6.0;
    #endif
    #ifdef DEF_3D
    float volume = (2 * PI * pow(radius, 6)) / 15.0;
    #endif
    return (radius - dist) * (radius - dist) / volume;
}

float smoothingKernelDerivative(float radius, float dist) {
    if (dist >= radius) return 0.0;
    #ifdef DEF_2D
    float scale = 12.0 / (PI * pow(radius, 4));
    #endif
    #ifdef DEF_3D
    float scale = 15.0 / (pow(radius, 5) * PI);
    #endif
    return (dist - radius) * scale;
}

void addDensity(inout float density, const float radius, const float mass, uint neighbourIndex, VEC_T neighbourPosition, float neighbourDistance) {
    float influence = smoothingKernel(radius, neighbourDistance);
    float contribution = mass * influence;
    density += contribution;
}

float evaluateDensity(VEC_T pos, float radius) {
    float density = 0.0;
    FOREACH_NEIGHBOUR(pos, {
        addDensity(density, radius, particleMass, NEIGHBOUR_INDEX, NEIGHBOUR_POSITION, NEIGHBOUR_DISTANCE);
    });
    return density;
}
