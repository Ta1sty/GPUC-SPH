#pragma once

#include "initialization.h"
#include "imgui_ui.h"
#include "debug_image.h"
#include "particle_physics.h"
#include "hashgrid.h"
#include "particle_renderer.h"

struct Particle {
    glm::vec2 position;
};

struct HashGridCell {
    uint32_t hashKey;
    uint32_t particleIndex;
};

// constant setting throughout the simulation
struct SimulationParameters {
    uint32_t numParticles;
};

class Camera;
class ParticleRenderer;
class HashGrid;
class ParticleSimulation;

// changed during simulation
struct SimulationState {
    UiBindings uiBindings;
    std::unique_ptr<Camera> camera;

    std::unique_ptr<DebugImage> debugImagePhysics;
    std::unique_ptr<DebugImage> debugImageSort;
    std::unique_ptr<DebugImage> debugImageRenderer;

    vk::Buffer particleBuffer;
    vk::DeviceMemory particleMemory;

    vk::Buffer hashGridBuffer;
    vk::DeviceMemory hashGridMemory;

    explicit SimulationState(const SimulationParameters &parameters);
};

// handles interop of the 3 parts, also copies rendered image to swapchain image
class Simulation {
    const SimulationParameters parameters;
    SimulationState state;

    std::unique_ptr<ImguiUi> imguiUi;

    std::unique_ptr<ParticleRenderer> particleRenderer;
    std::unique_ptr<HashGrid> hashGrid;
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
    explicit Simulation(SimulationParameters parameters);
    void run(uint32_t imageIndex, vk::Semaphore waitImageAvailable, vk::Semaphore signalRenderFinished, vk::Fence signalSubmitFinished);
};