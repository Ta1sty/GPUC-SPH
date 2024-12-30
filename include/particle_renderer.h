#pragma once

#include "initialization.h"

class ParticleRenderer {
    vk::Image colorAttachment;
    vk::ImageView colorAttachmentView;
    vk::DeviceMemory colorAttachmentMemory;
    vk::Framebuffer frameBuffers;

    vk::Pipeline renderPipeline;
public:
    explicit ParticleRenderer();
    vk::CommandBuffer run();
    [[nodiscard]] vk::Image getImage();
};