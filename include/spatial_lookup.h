#pragma once

#include "initialization.h"
#include "simulation_state.h"

class SpatialLookup {
    vk::Pipeline gridPipeline;
public:
    explicit SpatialLookup(const SimulationParameters &parameters);
    vk::CommandBuffer run(const SimulationState &state);
};