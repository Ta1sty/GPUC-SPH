#pragma once

#include "simulation_parameters.h"
#include "debug_image.h"
#include "render.h"
#include <random>


struct Particle {
    glm::vec2 position;
};

struct SpatialLookupEntry {
    uint32_t cellKey;
    uint32_t particleIndex;
};

/**
 * Physical state of the simulation (particle positions, forces, rng state, etc.).
 */
struct SimulationState {
public:
    SimulationState() = delete;
    SimulationState(const SimulationState &other) = delete; // don't accidentally copy
    explicit SimulationState(const SimulationParameters &parameters, std::shared_ptr<Camera> camera);
    ~SimulationState();

    [[nodiscard]] SimulationParameters getParameters() const { return parameters; }
    [[nodiscard]] uint32_t getCoordinateBufferSize() const { return coordinateBufferSize; }
    [[nodiscard]] const vk::Buffer& getParticleCoordinateBuffer() const { return particleCoordinateBuffer.buf; }

    Buffer particleCoordinateBuffer;

    const SimulationParameters parameters;
    std::shared_ptr<Camera> camera;

    std::unique_ptr<DebugImage> debugImagePhysics;
    std::unique_ptr<DebugImage> debugImageSort;
    std::unique_ptr<DebugImage> debugImageRenderer;


    Buffer spatialLookup;
    Buffer spatialIndices;

    uint32_t coordinateBufferSize = 0;
    std::mt19937 random;
    bool paused = false;
};
