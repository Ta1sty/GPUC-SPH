#pragma once

#include "initialization.h"

struct SimulationParameters;
struct RenderParameters;

/**
 * Wrap Parameters and update flags in convenience struct.
 */
struct UiBindings{
    uint32_t frameIndex;
    SimulationParameters &simulationParameters;
    RenderParameters &renderParameters;

    /**
     * Flags passed back to the simulation from the UI. Used to restart simulation, ...
     */
    struct UpdateFlags {
        bool resetSimulation = false;
    } updateFlags;

    /**
     * Wrap in constructor so you don't have to pass the update flags when initializing.
     */
    inline UiBindings(uint32_t frameIndex, SimulationParameters &simulationParameters,
                      RenderParameters &renderParameters) : frameIndex(frameIndex),
                                                            simulationParameters(simulationParameters),
                                                            renderParameters(renderParameters),
                                                            updateFlags() {}
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



