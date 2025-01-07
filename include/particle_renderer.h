#pragma once

#include "simulation_state.h"
#include "colormaps.h"

class ParticleRenderer {
public:
    ParticleRenderer() = delete;
    ParticleRenderer(const ParticleRenderer &particleRenderer) = delete;
    explicit ParticleRenderer(const SimulationParameters &simulationParameters);
    ~ParticleRenderer();
    vk::CommandBuffer run(const SimulationState &simulationState);
    [[nodiscard]] vk::Image getImage();

private:
    void createPipeline();
    void createColormapTexture(const std::vector<colormaps::RGB_F32> &colormap);

    struct PushStruct {
        uint32_t width = 0;
        uint32_t height = 0;
    } pushStruct;

    vk::Image colorAttachment;
    vk::ImageView colorAttachmentView;
    vk::DeviceMemory colorAttachmentMemory;
    vk::Framebuffer framebuffer;

    vk::Pipeline particlePipeline;
    vk::PipelineLayout particlePipelineLayout;
    vk::RenderPass renderPass;

    vk::CommandBuffer commandBuffer;

    vk::Image colormapImage;
    vk::DeviceMemory colormapImageMemory;
    vk::ImageView colormapImageView;
};