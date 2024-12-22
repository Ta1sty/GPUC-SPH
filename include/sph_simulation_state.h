//
// Created by Benedikt Krimmel on 12/20/2024.
//

#ifndef GPUC_PROJECT_SPH_SIMULATION_STATE_H
#define GPUC_PROJECT_SPH_SIMULATION_STATE_H

#include "sph_scene_parameters.h"
#include "initialization.h"
#include "utils.h"

#include <random>

namespace sph {

class SimulationState {
public:
    SimulationState() = default;
    void initialize(const SceneParameters &parameters, AppResources &app);

    void cleanup(AppResources &app);

    [[nodiscard]] SceneParameters getParameters() const { return parameters; }
    [[nodiscard]] uint32_t getCoordinateBufferSize() const { return coordinateBufferSize; }
    [[nodiscard]] const Buffer& getParticleCoordinateBuffer() const { return particleCoordinates; }

private:
    SceneParameters parameters;

    uint32_t coordinateBufferSize = 0;
    std::mt19937 random;

    Buffer particleCoordinates;
};

} // namespace sph

#endif //GPUC_PROJECT_SPH_SIMULATION_STATE_H
