#pragma once

#include "initialization.h"

class HashGrid {
    vk::Pipeline gridPipeline;
public:
    vk::CommandBuffer run();
};