#pragma once

#include "initialization.h"
#include "simulation_state.h"
#include "task_common.h"
#include "utils.h"


struct ParticleSimulationPushConstants {
    float gravity;
    float deltaTime;
    uint32_t numParticles;
    float collisionDamping;
    float spatialRadius;
    float targetDensity;
    float pressureMultiplier;
};


class ParticleSimulation {
public:
    ParticleSimulation() = delete;
    ParticleSimulation(const ParticleSimulation &particleSimulation) = delete;
    explicit ParticleSimulation(const SimulationParameters &parameters);
    ~ParticleSimulation();
    vk::CommandBuffer run(const SimulationState &simulationState);
    void updateCmd(const SimulationState &state);
    bool hasStateChanged(const SimulationState &state);
    vk::CommandBuffer cmd;

private:
    const uint32_t workgroupSizeX = 128;
    const uint32_t workgroupSizeY = 1;

    ParticleSimulationPushConstants currentPushConstants;

    vk::DescriptorSetLayout descriptorSetLayout;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorPool descriptorPool;

    vk::Pipeline computePipeline;
    vk::Pipeline densityPipeline;
    vk::PipelineLayout pipelineLayout;

    SimulationParameters simulationParameters;

    Buffer particleCoordinateBufferCopy;
    Buffer particleVelocityBufferCopy;
    Buffer particleDensityBufferCopy;
};