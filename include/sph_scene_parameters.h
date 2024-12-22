//
// Created by Benedikt Krimmel on 12/19/2024.
//

#ifndef GPUC_PROJECT_SPH_SCENE_PARAMETERS_H
#define GPUC_PROJECT_SPH_SCENE_PARAMETERS_H

#include <string>

namespace sph {

enum class SceneType {
    SPH_BOX_2D
};

enum class InitializationFunction {
    UNIFORM,
    POISSON_DISK
};

struct SceneParameters {
public:
    SceneType type = SceneType::SPH_BOX_2D;
    InitializationFunction initializationFunction = InitializationFunction::UNIFORM;
    uint32_t numParticles = 128;
    uint32_t randomSeed = 0; // initialized with TRNG if omitted

public:
    SceneParameters() = default;
    SceneParameters(const SceneParameters &other) = default;
    explicit SceneParameters(const std::string &file);
    [[nodiscard]] std::string printToYaml() const;
};

} // namespace sph

#endif //GPUC_PROJECT_SPH_SCENE_PARAMETERS_H
