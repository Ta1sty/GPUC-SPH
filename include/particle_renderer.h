#pragma once

#include "simulation_state.h"

class ParticleRenderer {
public:
    explicit ParticleRenderer(const SimulationParameters &simulationParameters);
    vk::CommandBuffer run(const SimulationState &simulationState);
    [[nodiscard]] vk::Image getImage();

private:
    vk::Image colorAttachment;
    vk::ImageView colorAttachmentView;
    vk::DeviceMemory colorAttachmentMemory;
    vk::Framebuffer framebuffer;

    vk::Pipeline particlePipeline;
    vk::RenderPass renderPass;

    vk::Buffer vertexInput;
    vk::DeviceMemory vertexInputMemory;

    vk::CommandBuffer commandBuffer;
};