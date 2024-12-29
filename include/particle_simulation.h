#pragma once

#include "simulation.h"

class ParticleSimulation {
    vk::Pipeline simulationPipeline;
public:
    vk::CommandBuffer run();
};