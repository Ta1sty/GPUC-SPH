#pragma once

#include "initialization.h"

class DebugImage {
    vk::DescriptorImageInfo descriptorInfo;
    vk::DeviceMemory imageMemory;

public:
    explicit DebugImage(std::string name);
    ~DebugImage();
    vk::Image image;
    vk::ImageView view;

    void clear(vk::CommandBuffer cmd, std::array<float, 4> color);
    vk::DescriptorSetLayoutBinding getLayout(uint32_t binding);
    vk::WriteDescriptorSet getWrite(vk::DescriptorSet set, uint32_t binding);
};
