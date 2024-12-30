#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <random>

#include "imgui_ui.h"
#include "initialization.h"
#include "parameters.h"

/**
 * Physical state of the simulation (particle positions, forces, rng state, etc.).
 */
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

class Camera;
class ParticleRenderer;
class HashGrid;
class ParticleSimulation;

// changed during simulation
struct SimulationState {
    UiBindings uiBindings;
    std::unique_ptr<Camera> camera;
public:
    SimulationState() = delete;
    SimulationState(const SimulationState &other) = delete; // don't accidentally copy
    SimulationState(const SimulationParameters &parameters);
    ~SimulationState();

    std::unique_ptr<DebugImage> debugImagePhysics;
    std::unique_ptr<DebugImage> debugImageSort;
    std::unique_ptr<DebugImage> debugImageRenderer;
    [[nodiscard]] SimulationParameters getParameters() const { return parameters; }
    [[nodiscard]] uint32_t getCoordinateBufferSize() const { return coordinateBufferSize; }
    [[nodiscard]] const Buffer& getParticleCoordinateBuffer() const { return particleCoordinates; }

    vk::Buffer particleBuffer;
    vk::DeviceMemory particleMemory;
    SimulationParameters parameters;

    vk::Buffer hashGridBuffer;
    vk::DeviceMemory hashGridMemory;
    uint32_t coordinateBufferSize = 0;
    std::mt19937 random;
};

// handles interop of the 3 parts, also copies rendered image to swapchain image
class Simulation {
public:
    Simulation();
    ~Simulation() = default;

    void run();

private:
    const SimulationParameters parameters;

    std::unique_ptr<ImguiUi> imguiUi;

    std::unique_ptr<ParticleRenderer> particleRenderer;
    std::unique_ptr<SimulationState> state;
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
    std::unique_ptr<ParticleSimulation> particleSimulation;
};
