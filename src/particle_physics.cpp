#include "particle_physics.h"
#include "simulation_state.h"
#include "task_common.h"

ParticleSimulation::ParticleSimulation(const SimulationParameters &simulationParameters)
{
    vk::ShaderModule particleComputeSM;
    Cmn::createShader(resources.device, particleComputeSM, shaderPath("particle_simulation.comp"));
    vk::PipelineShaderStageCreateInfo computeShaderStageInfo{};

    computeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    computeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
    computeShaderStageInfo.module = particleComputeSM;
    computeShaderStageInfo.pName = "main";
}

vk::CommandBuffer ParticleSimulation::run(const SimulationState &simulationState)
{
    return nullptr;
}
