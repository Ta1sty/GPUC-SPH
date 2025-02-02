#pragma once

#include <map>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "initialization.h"
#include "simulation_state.h"

#include "debug_image.h"
#include "imgui_ui.h"
#include "particle_physics.h"
#include "particle_renderer.h"
#include "spatial_lookup.h"

// handles interop of the 3 parts, also copies rendered image to swapchain image
class Simulation {
public:
    Simulation() = delete;
    explicit Simulation(std::shared_ptr<Camera> camera, const std::string &sceneFile = {});
    ~Simulation();

    void reset();
    void updateTimestamps();
    bool updateTime();
    void run(uint32_t imageIndex, vk::Semaphore waitImageAvailable, vk::Semaphore signalRenderFinished, vk::Fence signalSubmitFinished);
    SimulationState &getState() { return *simulationState; }
    const QueryTimes &getQueryTimes() { return queryTimes; }

private:
    std::map<Query, double> timestamps;
    double prevTime = 0;

    void processUpdateFlags(const UpdateFlags &updateFlags);
    void updateCommandBuffers();

    RenderParameters renderParameters;
    SimulationParameters simulationParameters;
    std::unique_ptr<SimulationState> simulationState;

    std::unique_ptr<ImguiUi> imguiUi;

    std::unique_ptr<ParticleSimulation> particlePhysics;
    std::unique_ptr<SpatialLookup> spatialLookup;
    std::unique_ptr<RendererCompute> rendererCompute;
    std::unique_ptr<ParticleRenderer> particleRenderer;

    // clears the color values for the debug images
    vk::CommandBuffer cmdReset;

    // copies the generated rendered image to the swapchain image
    vk::CommandBuffer cmdCopy;
    // an empty command buffer
    vk::CommandBuffer cmdEmpty;

    vk::Semaphore timelineSemaphore;

    vk::Semaphore initSemaphore();
    vk::CommandBuffer copy(uint32_t imageIndex);

    UpdateFlags lastUpdate;
    QueryTimes queryTimes = QueryTimes(timestamps, queryTimes);

    void check();

public:
    std::unique_ptr<ParticleSimulation> particleSimulation;
};
