#pragma once

#include <cstdlib>
#include <iostream>

#include "initialization.h"
#include "utils.h"
#include <fstream>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Cmn {
void createDescriptorSetLayout(vk::Device &device,
                               std::vector<vk::DescriptorSetLayoutBinding> &bindings, vk::DescriptorSetLayout &descLayout);
void addStorage(std::vector<vk::DescriptorSetLayoutBinding> &bindings, uint32_t binding);
void addCombinedImageSampler(std::vector<vk::DescriptorSetLayoutBinding> &bindings, uint32_t binding);

void allocateDescriptorSet(vk::Device &device, vk::DescriptorSet &descSet, vk::DescriptorPool &descPool,
                           vk::DescriptorSetLayout &descLayout);

void bindCombinedImageSampler(vk::Device &device, vk::ImageView &view, vk::Sampler &sampler, vk::DescriptorSet &set, uint32_t binding);
void bindBuffers(vk::Device &device, const vk::Buffer &b, vk::DescriptorSet &set, uint32_t binding);

void createDescriptorPool(vk::Device &device, std::vector<vk::DescriptorSetLayoutBinding> &bindings, vk::DescriptorPool &descPool, uint32_t numDescriptorSets = 1);
void createPipeline(vk::Device &device, vk::Pipeline &pipeline,
                    vk::PipelineLayout &pipLayout, vk::SpecializationInfo &specInfo, vk::ShaderModule &sModule);
void createShader(vk::Device &device, vk::ShaderModule &shaderModule, const std::string &filename);

}// namespace Cmn

struct TaskResources {
    //std::vector<Buffer> buffers; move this to user code
    vk::ShaderModule cShader;

    vk::DescriptorSetLayout descriptorSetLayout;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorPool descriptorPool;

    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;

    void destroy(vk::Device &device);
};
