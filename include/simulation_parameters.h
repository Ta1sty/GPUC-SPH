#pragma once

#include "utils.h"

enum class SceneType
{
    SPH_BOX_2D
};

enum class InitializationFunction
{
    UNIFORM,
    POISSON_DISK
};

/**
 * Parameters that influence setup and execution of the simulation.
 * Changing parameters requires the simulation to be restarted.
 */
struct SimulationParameters
{
public:
    SceneType type = SceneType::SPH_BOX_2D;
    InitializationFunction initializationFunction = InitializationFunction::UNIFORM;
    uint32_t numParticles = 128;
    uint32_t randomSeed = 0; // initialized with TRNG if omitted
    float gravity = -9.81f;  // Default Earth gravity

public:
    SimulationParameters() = default;
    SimulationParameters(const SimulationParameters &other) = default;
    explicit SimulationParameters(const std::string &file);
    [[nodiscard]] std::string printToYaml() const;
};

/**
 * Parameters that only influence the visualization of the Simulation.
 */
struct RenderParameters
{
    bool showDemoWindow = false;
    bool debugImagePhysics = false;
    bool debugImageSort = false;
    bool debugImageRenderer = false;
};
