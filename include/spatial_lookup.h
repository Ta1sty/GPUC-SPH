#pragma once

#include "initialization.h"
#include "simulation_state.h"

struct SpatialLookupPushConstants {
    uint32_t bufferSize;
    float cellSize;
};

class SpatialLookup {
    SpatialLookupPushConstants pushConstants {0, 0};

    uint32_t workgroupSizeX = -1;

    std::vector<vk::DescriptorSetLayoutBinding> descriptorBindings;
    vk::DescriptorSetLayout descriptorLayout;
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;

    vk::PipelineLayout pipelineLayout;

    vk::ShaderModule writeShader;
    vk::Pipeline writePipeline = nullptr;

    vk::ShaderModule sortShader;
    vk::Pipeline sortPipeline = nullptr;

    vk::ShaderModule indexShader;
    vk::Pipeline indexPipeline = nullptr;

    vk::CommandBuffer cmd;

    void createPipelines();

public:
    explicit SpatialLookup(const SimulationParameters &parameters);
    ~SpatialLookup();
    void updateCmd(const SimulationState &state);
    vk::CommandBuffer run(SimulationState &state);
};