#pragma once

#include "simulation.h"

class HashGrid {
    vk::Pipeline gridPipeline;
public:
    vk::CommandBuffer run();
};