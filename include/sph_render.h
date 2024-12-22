//
// Created by Benedikt Krimmel on 12/20/2024.
//

#ifndef GPUC_PROJECT_SPH_RENDER_H
#define GPUC_PROJECT_SPH_RENDER_H

#include "initialization.h"
#include "sph_simulation_state.h"

namespace sph {

class Render2D {
public:
    Render2D();

    void init(AppResources &app, const SimulationState &state);

    void cleanup(AppResources &app);

    void render(const SimulationState &state);

private:
    vk::Pipeline particlePipeline;
    vk::Pipeline backgroundFieldPipeline;
};

class Render {
public:
    Render();

    void init(AppResources &app, const SimulationState &state);

    void cleanup(AppResources &app);

    void render(const SimulationState &state);

private:
    Render2D render2D;
};

} // namespace sph

#endif //GPUC_PROJECT_SPH_RENDER_H
