//
// Created by Benedikt Krimmel on 12/20/2024.
//

#ifndef GPUC_PROJECT_SPH_SIMULATION_STATE_H
#define GPUC_PROJECT_SPH_SIMULATION_STATE_H

#include "sph_scene_parameters.h"
#include "initialization.h"
#include "utils.h"

#include <random>

class SPHSimulationState {
public:
    SPHSimulationState() = default;
    void initialize(const SPHSceneParameters &parameters, AppResources &app);

    void cleanup(AppResources &app);

private:
    uint32_t numParticles = 0;
    uint32_t coordinateBufferSize = 0;
    SceneType type = SceneType::SPH_BOX_2D;
    std::mt19937 random;

    Buffer particleCoordinates;
};


#endif //GPUC_PROJECT_SPH_SIMULATION_STATE_H
