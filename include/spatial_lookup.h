#pragma once

#include "initialization.h"
#include "simulation_state.h"

class SpatialLookup {
    uint32_t workgroupSizeX = 128;

    std::vector<vk::DescriptorSetLayoutBinding> descriptorBindings;
    vk::DescriptorSetLayout descriptorLayout;
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;

    vk::PipelineLayout pipelineLayout;

    vk::ShaderModule writeShader;
    vk::Pipeline writePipeline;

    vk::ShaderModule sortShader;
    vk::Pipeline sortPipeline;

    vk::ShaderModule indexShader;
    vk::Pipeline indexPipeline;

    vk::CommandBuffer cmd;

public:
    explicit SpatialLookup(const SimulationParameters &parameters);
    void updateCmd(const SimulationState &state);
    vk::CommandBuffer run(SimulationState &state);
};