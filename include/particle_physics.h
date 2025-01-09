#pragma once

#include "initialization.h"

class ParticleSimulation
{
    vk::Pipeline simulationPipeline;
    vk::PipelineLayout simulationPipelineLayout;

public:
    ParticleSimulation() = delete;
    ParticleSimulation(const ParticleSimulation &particleSimulation) = delete;
    explicit ParticleSimulation(const SimulationParameters &simulationParameters);
    ~ParticleSimulation();
    vk::CommandBuffer run(const SimulationState &simulationState);
};