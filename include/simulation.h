#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <random>

#include "imgui_ui.h"
#include "initialization.h"
#include "parameters.h"

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

/**
 * Physical state of the simulation (particle positions, forces, rng state, etc.).
 */
struct SimulationState {
public:
    SimulationState() = delete;
    SimulationState(const SimulationState &other) = delete; // don't accidentally copy
    SimulationState(const SimulationParameters &parameters);
    ~SimulationState();

    [[nodiscard]] SimulationParameters getParameters() const { return parameters; }
    [[nodiscard]] uint32_t getCoordinateBufferSize() const { return coordinateBufferSize; }
    [[nodiscard]] const vk::Buffer& getParticleCoordinateBuffer() const { return particleCoordinateBuffer; }

    vk::Buffer particleCoordinateBuffer;
    vk::DeviceMemory particleMemory;
    SimulationParameters parameters;

    std::unique_ptr<DebugImage> debugImagePhysics;
    std::unique_ptr<DebugImage> debugImageSort;
    std::unique_ptr<DebugImage> debugImageRenderer;

    std::unique_ptr<Camera> camera;
    vk::Buffer hashGridBuffer;
    vk::DeviceMemory hashGridMemory;
    uint32_t coordinateBufferSize = 0;
    std::mt19937 random;
};

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
    std::unique_ptr<ParticleSimulation> particleSimulation;
};
