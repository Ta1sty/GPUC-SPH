#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>

#include "simulation_state.h"
#include "initialization.h"

#include "imgui_ui.h"
#include "debug_image.h"
#include "particle_physics.h"
#include "spatial_lookup.h"
#include "particle_renderer.h"

// handles interop of the 3 parts, also copies rendered image to swapchain image
class Simulation {
public:
    Simulation() = delete;
    explicit Simulation(const SimulationParameters &parameters);
    ~Simulation();

    void run(uint32_t imageIndex, vk::Semaphore waitImageAvailable, vk::Semaphore signalRenderFinished, vk::Fence signalSubmitFinished);

private:
    SimulationParameters simulationParameters;
    RenderParameters renderParameters;

    std::unique_ptr<ImguiUi> imguiUi;

    std::unique_ptr<ParticleRenderer> particleRenderer;
    std::unique_ptr<SimulationState> simulationState;
    std::unique_ptr<SpatialLookup> hashGrid;
    std::unique_ptr<ParticleSimulation> particlePhysics;

    // clears the color values for the debug images
    vk::CommandBuffer cmdReset;

    // copies the generated rendered image to the swapchain image
    vk::CommandBuffer cmdCopy;
    // an empty command buffer
    vk::CommandBuffer cmdEmpty;

    vk::Semaphore timelineSemaphore;
    vk::Fence timelineFence;

    vk::Semaphore initSemaphore();
    vk::CommandBuffer copy(uint32_t imageIndex);
public:
    std::unique_ptr<ParticleSimulation> particleSimulation;
};
