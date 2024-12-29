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
struct SimulationState {
public:
    SimulationState() = delete;
    SimulationState(const SimulationState &other) = delete; // don't accidentally copy
    SimulationState(const SimulationParameters &parameters);
    ~SimulationState();

    [[nodiscard]] SimulationParameters getParameters() const { return parameters; }
    [[nodiscard]] uint32_t getCoordinateBufferSize() const { return coordinateBufferSize; }
    [[nodiscard]] const Buffer& getParticleCoordinateBuffer() const { return particleCoordinates; }

private:
    SimulationParameters parameters;

    uint32_t coordinateBufferSize = 0;
    std::mt19937 random;

    Buffer particleCoordinates;
};

class HashGrid;
class ParticleSimulation;
class Renderer;

// handles interop of the 3 parts, also copies rendered image to swapchain image
class Simulation {
public:
    Simulation();
    ~Simulation() = default;

    void run();

private:
    const SimulationParameters parameters;

    ImguiUi imguiUi;
    Camera camera;

    std::unique_ptr<SimulationState> state;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<HashGrid> hashGrid;
    std::unique_ptr<ParticleSimulation> particleSimulation;
};
