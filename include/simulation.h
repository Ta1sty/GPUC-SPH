#pragma once

#include "imgui_ui.h"
#include <vulkan/vulkan.hpp>
#include <memory>
#include "initialization.h"

// constant setting throughout the simulation
struct SimulationParameters {

};

// changed during simulation
struct SimulationState {
    UiBindings uiBindings;

    vk::Image debugImage1;
    vk::Image debugImage2;

    vk::Buffer particles;

    vk::Buffer hashGridBuffer;

};

class Renderer;
class HashGrid;
class ParticleSimulation;

// handles interop of the 3 parts, also copies rendered image to swapchain image
class Simulation {
    const SimulationParameters parameters;
    SimulationState state;

    ImguiUi imguiUi;
    Camera camera;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<HashGrid> hashGrid;
    std::unique_ptr<ParticleSimulation> particleSimulation;

    void run();
};