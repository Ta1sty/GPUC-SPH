#pragma once

#include "simulation.h"

class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();

    vk::CommandBuffer run(const SimulationState &state);

private:
    vk::Pipeline particlePipeline;
    vk::Pipeline backgroundFieldPipeline;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    vk::CommandBuffer run(const SimulationState &state);

private:
    Renderer2D renderer2D;
};