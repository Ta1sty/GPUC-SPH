#pragma once

#include "initialization.h"
#include "simulation_state.h"
#include "task_common.h"


class ParticleSimulation {
public:
    ParticleSimulation() = delete;
    ParticleSimulation(const ParticleSimulation &particleSimulation) = delete;
    explicit ParticleSimulation(const SimulationParameters &parameters);
    ~ParticleSimulation();
    vk::CommandBuffer run(const SimulationState &simulationState);
    void updateCmd(const SimulationState &state);

private:
    const uint32_t workgroupSizeX = 128;
    const uint32_t workgroupSizeY = 1;
    TaskResources classResources;
    SimulationParameters simulationParameters;
    vk::CommandBuffer cmd;
    struct PushStruct {
        float gravity;
        float deltaTime;
        uint32_t numParticles;
        float collisionDamping;
        float particleRadius;
        float spatialRadius;
    } pushStruct;
};