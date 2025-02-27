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
    float viscosity;
    float boundaryThreshold;
    float boundaryForceStrength;
};


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

    ParticleSimulationPushConstants currentPushConstants;
    SceneType currentSceneType;

    vk::CommandBuffer cmd;

    vk::DescriptorSetLayout descriptorSetLayout;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorPool descriptorPool;

    vk::Pipeline computePipeline;
    vk::Pipeline densityPipeline;
    vk::Pipeline positionUpdatePipeline;
    vk::PipelineLayout pipelineLayout;

    SimulationParameters simulationParameters;

    Buffer particleVelocityBufferCopy;


    bool hasStateChanged(const SimulationState &state);
    void createShaderPipelines(const SceneType newType);
};