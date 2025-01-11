#pragma once

#include "debug_image.h"
#include "render.h"
#include "simulation_parameters.h"
#include <random>


struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
};

struct SpatialLookupEntry {
    uint32_t cellKey;
    uint32_t particleIndex;
};

struct SimulationTime {
    double time;
    long ticks = 0;
    int tickRate = 25;
    double lastUpdate;
    bool advance(double add);
};

/**
 * Physical state of the simulation (particle positions, forces, rng state, etc.).
 */
struct SimulationState {
public:
    SimulationState() = delete;
    SimulationState(const SimulationState &other) = delete;// don't accidentally copy
    explicit SimulationState(const SimulationParameters &parameters, std::shared_ptr<Camera> camera);
    ~SimulationState();

    [[nodiscard]] SimulationParameters getParameters() const { return parameters; }
    [[nodiscard]] uint32_t getCoordinateBufferSize() const { return coordinateBufferSize; }
    [[nodiscard]] const vk::Buffer &getParticleCoordinateBuffer() const { return particleCoordinateBuffer.buf; }

    SimulationTime time;
    Buffer particleCoordinateBuffer;
    Buffer particleVelocityBuffer;

    const SimulationParameters parameters;
    std::shared_ptr<Camera> camera;

    std::unique_ptr<DebugImage> debugImagePhysics;
    std::unique_ptr<DebugImage> debugImageSort;
    std::unique_ptr<DebugImage> debugImageRenderer;


    Buffer spatialLookup;
    Buffer spatialIndices;

    uint32_t coordinateBufferSize = 0;
    uint32_t velocityBufferSize = 0;
    std::mt19937 random;
    bool paused = false;
    bool step = false;
};
