#pragma once

#include "utils.h"
#include <string>

namespace YAML {
class Node;
}

template<typename T>
using Mappings = std::initializer_list<std::pair<std::string, T>>;

template<typename T>
std::vector<const char *> imguiComboArray(const Mappings<T> mappings) {
    std::vector<const char *> result {mappings.size()};
    for (const auto &[label, v]: mappings) {
        result[static_cast<size_t>(v)] = label.c_str();
    }

    return result;
}

extern const Mappings<SceneType> sceneTypeMappings;

enum class InitializationFunction {
    UNIFORM,
    POISSON_DISK,
    JITTERED
};
extern const Mappings<InitializationFunction> initializationFunctionMappings;

enum class RenderParticleColor {
    NONE,
    WHITE,
    NUM_NEIGHBOURS,
    DENSITY,
    VELOCITY,
};

/**
 * Parameters that influence setup and execution of the simulation.
 * Changing parameters requires the simulation to be restarted.
 */
struct SimulationParameters {
public:
    SceneType type = SceneType::SPH_BOX_2D;
    InitializationFunction initializationFunction = InitializationFunction::UNIFORM;
    uint32_t numParticles = 128;
    uint32_t randomSeed = 0;            // initialized with TRNG if omitted
    float gravity = 9.81f;              // Default Earth gravity
    float deltaTime = 1.0f / 60.0f;     // Default 60 FPS
    float collisionDampingFactor = 0.0f;// Default energy loss
    float targetDensity = 2.75f;
    float pressureMultiplier = 0.5f;
    float viscosity = 0.0f;
    float spatialRadius = 0.05f;
    float boundaryThreshold = 0.05f;
    float boundaryForceStrength = 1000.0f;

public:
    SimulationParameters() = default;
    SimulationParameters(const SimulationParameters &other) = default;
    explicit SimulationParameters(const YAML::Node &yaml);
    [[nodiscard]] std::string printToYaml() const;
};

enum class SelectedImage {
    RENDER,
    DEBUG_PHYSICS,
    DEBUG_SORT,
    DEBUG_RENDERER
};
extern const Mappings<SelectedImage> selectedImageMappings;

enum class RenderBackgroundField {
    NONE,
    CELL_HASH,
    CELL_CLASS,
    DENSITY,
    VELOCITY,
    WATER,
};
extern const Mappings<RenderBackgroundField> renderBackgroundFieldMappings;

extern const Mappings<RenderParticleColor> renderParticleColorMappings;

/**
 * Parameters that only influence the visualization of the Simulation.
 */
struct RenderParameters {
    bool showDemoWindow = false;
    SelectedImage selectedImage = SelectedImage::RENDER;

    bool backgroundEnvironment = true;
    RenderBackgroundField backgroundField = RenderBackgroundField::DENSITY;
    RenderParticleColor particleColor = RenderParticleColor::WHITE;

    float particleRadius = 12.0f;
    std::string densityGridShader = "density_grid.comp.3D";// don't set via UI, is only used during pipeline initializations
    glm::uvec3 densityGridWGSize = {8, 8, 8};

public:
    RenderParameters() = default;
    RenderParameters(const RenderParameters &other) = default;
    explicit RenderParameters(const YAML::Node &yaml);
    [[nodiscard]] std::string printToYaml() const;
};

class SceneParameters {
public:
    static std::pair<RenderParameters, SimulationParameters> loadParametersFromFile(const std::string sceneFile);
};
