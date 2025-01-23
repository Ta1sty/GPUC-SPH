#pragma once

#include "debug_image.h"
#include "render.h"
#include "simulation_parameters.h"
#include <random>


struct SpatialLookupEntry {
    uint32_t cellKey;
    uint32_t cellClass;
    uint32_t particleIndex;
};

struct SimulationTime {
    double time = 0.0;
    long ticks = 0;
    int tickRate = 25;
    double lastUpdate = 0.0;
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

    SimulationTime time;
    Buffer particleCoordinateBuffer;
    Buffer particleVelocityBuffer;
    Buffer particleDensityBuffer;

    const SimulationParameters parameters;
    std::shared_ptr<Camera> camera;

    std::unique_ptr<DebugImage> debugImagePhysics;
    std::unique_ptr<DebugImage> debugImageSort;
    std::unique_ptr<DebugImage> debugImageRenderer;

    // the radius in which particles are considered close to each other, also the cell size for the spatial-lookup
    float spatialRadius = 0.1f;
    Buffer spatialLookup;
    Buffer spatialIndices;

    std::mt19937 random;
    bool paused = true;
    bool step = false;
};
