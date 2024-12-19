//
// Created by Benedikt Krimmel on 12/19/2024.
//

#ifndef GPUC_PROJECT_SPH_SCENE_PARAMETERS_H
#define GPUC_PROJECT_SPH_SCENE_PARAMETERS_H

#include <string>

enum class SceneType {
    SPH_BOX_2D // sph_box_2d
};

enum class InitializationFunction {
    UNIFORM_POISSON_DISK // uniform_poisson_disk
};

struct SPHSceneParameters {
    SceneType type = SceneType::SPH_BOX_2D;
    InitializationFunction initializationFunction = InitializationFunction::UNIFORM_POISSON_DISK;
    uint32_t numParticles = 128;

    SPHSceneParameters() = default;
    SPHSceneParameters(const SPHSceneParameters &other) = default;
    explicit SPHSceneParameters(const std::string &file);
    std::string printToYaml() const;
};


#endif //GPUC_PROJECT_SPH_SCENE_PARAMETERS_H
