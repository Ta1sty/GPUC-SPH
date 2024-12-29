#pragma once

#include "initialization.h"

class ParticleSimulation {
    vk::Pipeline simulationPipeline;
public:
    vk::CommandBuffer run();
};