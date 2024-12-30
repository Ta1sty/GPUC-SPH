#pragma once

#include "initialization.h"

// put ui bindings here
struct UiBindings{
    uint32_t frameIndex;
    bool sampleCheckbox;

    bool debugImagePhysics;
    bool debugImageSort;
    bool debugImageRenderer;
};

class ImguiUi {
    vk::DescriptorPool descriptorPool;
    vk::RenderPass renderPass;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Framebuffer> frameBuffers;

public:
    explicit ImguiUi();
    ~ImguiUi();

    void initCommandBuffers();

    vk::CommandBuffer updateCommandBuffer(uint32_t index, UiBindings &bindings);
    void destroyCommandBuffers();

    void drawUi(UiBindings &bindings);
};



