#pragma once

#include "simulation_parameters.h"
#include "debug_image.h"
#include "render.h"
#include <random>


struct Particle {
    glm::vec2 position;
};

struct HashGridCell {
    uint32_t hashKey;
    uint32_t particleIndex;
};

/**
 * Physical state of the simulation (particle positions, forces, rng state, etc.).
 */
struct SimulationState {
public:
    SimulationState() = delete;
    SimulationState(const SimulationState &other) = delete; // don't accidentally copy
    explicit SimulationState(const SimulationParameters &parameters);
    ~SimulationState();

    [[nodiscard]] SimulationParameters getParameters() const { return parameters; }
    [[nodiscard]] uint32_t getCoordinateBufferSize() const { return coordinateBufferSize; }
    [[nodiscard]] const vk::Buffer& getParticleCoordinateBuffer() const { return particleCoordinateBuffer.buf; }

    Buffer particleCoordinateBuffer;

    const SimulationParameters parameters;

    std::unique_ptr<DebugImage> debugImagePhysics;
    std::unique_ptr<DebugImage> debugImageSort;
    std::unique_ptr<DebugImage> debugImageRenderer;

    std::unique_ptr<Camera> camera;

    // Spatial lookup
    Buffer spatialLookup;
    Buffer startIndices;

    uint32_t coordinateBufferSize = 0;
    std::mt19937 random;
};
