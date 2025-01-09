#pragma once

#include "utils.h"
#include <string>

template <typename T>
using Mappings = std::initializer_list<std::pair<std::string, T>>;

template <typename T>
std::vector<const char*> imguiComboArray(const Mappings<T> mappings) {
    std::vector<const char*> result { mappings.size() };
    for (const auto &[label, v] : mappings) {
        result[static_cast<size_t>(v)] = label.c_str();
    }

    return result;
}

enum class SceneType {
    SPH_BOX_2D

};
extern const Mappings<SceneType> sceneTypeMappings;

enum class InitializationFunction {
    UNIFORM,
    POISSON_DISK
};
extern const Mappings<InitializationFunction> initializationFunctionMappings;

/**
 * Parameters that influence setup and execution of the simulation.
 * Changing parameters requires the simulation to be restarted.
 */
struct SimulationParameters {
public:
    SceneType type = SceneType::SPH_BOX_2D;
    InitializationFunction initializationFunction = InitializationFunction::UNIFORM;
    uint32_t numParticles = 128;
    uint32_t randomSeed = 0; // initialized with TRNG if omitted

public:
    SimulationParameters() = default;
    SimulationParameters(const SimulationParameters &other) = default;
    explicit SimulationParameters(const std::string &file);
    [[nodiscard]] std::string printToYaml() const;
};

enum class RenderBackgroundField {
    CELL_HASH,
    DENSITY,
};
extern const Mappings<RenderBackgroundField> renderBackgroundFieldMappings;

/**
 * Parameters that only influence the visualization of the Simulation.
 */
struct RenderParameters {
    bool showDemoWindow = false;
    bool debugImagePhysics = false;
    bool debugImageSort = false;
    bool debugImageRenderer = false;

    RenderBackgroundField backgroundField = RenderBackgroundField::CELL_HASH;
};
