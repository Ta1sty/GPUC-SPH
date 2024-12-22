#pragma once

#include "simulation.h"

class Renderer {
    vk::Image outputImages;
    vk::Framebuffer frameBuffers;

    vk::Pipeline renderPipeline;
public:
    vk::CommandBuffer run();
};