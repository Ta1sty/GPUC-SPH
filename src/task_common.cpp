#include <cstdlib>
#include <iostream>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "initialization.h"
#include "task_common.h"
#include "utils.h"
#include <fstream>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Cmn {
//We have a binding vector ready to become a descriptorSetLayout
void createDescriptorSetLayout(vk::Device &device,
                               std::vector<vk::DescriptorSetLayoutBinding> &bindings, vk::DescriptorSetLayout &descLayout) {
    vk::DescriptorSetLayoutCreateInfo layoutInfo(
            {},
            CAST(bindings),// Number of binding infos
            bindings.data()// Array of binding infos
    );
    descLayout = device.createDescriptorSetLayout(layoutInfo);
}

void addStorage(std::vector<vk::DescriptorSetLayoutBinding> &bindings, uint32_t binding) {
    //Binding Info
    //Bindings needed for DescriptorSetLayout
    //The DescriptorType eStorageBuffer is used in our case as storage buffer for compute shader
    //The ID binding(argument) is needed in the shader
    //DescriptorCount is set to 1U
    bindings.push_back(vk::DescriptorSetLayoutBinding(
            binding,                           // The binding number of this entry
            vk::DescriptorType::eStorageBuffer,// Type of resource descriptors used for this binding
            1U,                                // Number of descriptors contained in the binding
            vk::ShaderStageFlagBits::eAll)     // All defined shader stages can access the resource
    );
}

void addCombinedImageSampler(std::vector<vk::DescriptorSetLayoutBinding> &bindings, uint32_t binding) {
    //Binding Info
    bindings.push_back(vk::DescriptorSetLayoutBinding(
            binding,                                  // The binding number of this entry
            vk::DescriptorType::eCombinedImageSampler,// Type of resource descriptors used for this binding
            1U,                                       // Number of descriptors contained in the binding
            vk::ShaderStageFlagBits::eAll)            // All defined shader stages can access the resource
    );
}

void allocateDescriptorSet(vk::Device &device, vk::DescriptorSet &descSet, vk::DescriptorPool &descPool,
                           vk::DescriptorSetLayout &descLayout) {
    // You can technically allocate multiple layouts at once, we don't need that (so we put 1)
    vk::DescriptorSetAllocateInfo descAllocInfo(descPool, 1U, &descLayout);
    // Therefore the vector is length one, we want to take its (only) element
    descSet = device.allocateDescriptorSets(descAllocInfo)[0];
}

void bindCombinedImageSampler(vk::Device &device, vk::ImageView &view, vk::Sampler &sampler, vk::DescriptorSet &set, uint32_t binding) {
    // Colour Attachment Descriptor
    vk::DescriptorImageInfo imageInfo(sampler, view,
                                      vk::ImageLayout::eShaderReadOnlyOptimal);
    // Colour Attachment Descriptor Write
    vk::WriteDescriptorSet write(set, binding, 0U, 1U,
                                 vk::DescriptorType::eCombinedImageSampler, &imageInfo);
    device.updateDescriptorSets(1U, &write, 0U, nullptr);
}

void bindBuffers(vk::Device &device, const vk::Buffer &b, vk::DescriptorSet &set, uint32_t binding, vk::DescriptorType type) {
    // Buffer info and data offset info
    vk::DescriptorBufferInfo descInfo(
            b,           // Buffer to get data from
            0ULL,        // Position of start of data
            VK_WHOLE_SIZE// Size of data
    );
    //  Binding index in the shader    V
    //  Data about connection between binding and buffer
    vk::WriteDescriptorSet write(
            set,    // Descriptor Set to update
            binding,// Binding to update (matches with binding on layout/shader)
            0U,     // Index in array to update
            1U,     // Amount to update
            type,   // Type of descriptor
            nullptr,
            &descInfo// Information about buffer data to bind
    );

    // Update the descriptor sets with new buffer/binding info
    device.updateDescriptorSets(1U, &write, 0U, nullptr);
}

void createPipeline(vk::Device &device, vk::Pipeline &pipeline,
                    vk::PipelineLayout &pipLayout, vk::SpecializationInfo &specInfo,
                    vk::ShaderModule &sModule) {
    vk::PipelineShaderStageCreateInfo stageInfo(vk::PipelineShaderStageCreateFlags(),
                                                vk::ShaderStageFlagBits::eCompute, sModule,
                                                "main", &specInfo);

    vk::ComputePipelineCreateInfo computeInfo(vk::PipelineCreateFlags(), stageInfo, pipLayout);

    // This is a workaround: ideally there should not be a ".value"
    // This should be fixed in later releases of the SDK
    pipeline = device.createComputePipeline(nullptr, computeInfo, nullptr).value;
}
//Number of DescriptorSets is one by default
void createDescriptorPool(vk::Device &device, std::vector<vk::DescriptorSetLayoutBinding> &bindings, vk::DescriptorPool &descPool, uint32_t numDescriptorSets) {
    uint32_t numStorage = 0, numCombinedImageSampler = 0, numUniform = 0;

    for (const auto &binding: bindings) {
        switch (binding.descriptorType) {
            case vk::DescriptorType::eStorageBuffer:
                numStorage++;
                break;
            case vk::DescriptorType::eCombinedImageSampler:
                numCombinedImageSampler++;
                break;
            case vk::DescriptorType::eUniformBuffer:
                numUniform++;
                break;
            default:
                break;
        }
    }

    // List of pool sizes
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    if (numStorage > 0)
        descriptorPoolSizes.push_back(vk::DescriptorPoolSize {
                vk::DescriptorType::eStorageBuffer, numStorage * numDescriptorSets});
    if (numCombinedImageSampler > 0)
        descriptorPoolSizes.push_back(vk::DescriptorPoolSize {
                vk::DescriptorType::eCombinedImageSampler, numCombinedImageSampler * numDescriptorSets});
    if (numUniform > 0)
        descriptorPoolSizes.push_back(vk::DescriptorPoolSize {
                vk::DescriptorType::eUniformBuffer, numUniform * numDescriptorSets});

    // Data to create Descriptor Pool
    vk::DescriptorPoolCreateInfo descriptorPoolCI = vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlags(), numDescriptorSets, descriptorPoolSizes);

    descPool = device.createDescriptorPool(descriptorPoolCI);
}

void createShader(vk::Device &device, vk::ShaderModule &shaderModule, const std::string &filename) {
    std::vector<char> cshader = readFile(filename);
    // Shader Module creation information
    vk::ShaderModuleCreateInfo smi(
            {},
            static_cast<uint32_t>(cshader.size()),              // Size of code
            reinterpret_cast<const uint32_t *>(cshader.data()));// Pointer to code (of uint32_t pointer type)
    shaderModule = device.createShaderModule(smi);
}
}// namespace Cmn

void TaskResources::destroy(vk::Device &device) {
    //Destroy all the resources we created in reverse order
    //Pipeline Should be destroyed before PipelineLayout
    device.destroyPipeline(this->pipeline);
    //PipelineLayout should be destroyed before DescriptorPool
    device.destroyPipelineLayout(this->pipelineLayout);
    //DescriptorPool should be destroyed before the DescriptorSetLayout
    device.destroyDescriptorPool(this->descriptorPool);
    device.destroyDescriptorSetLayout(this->descriptorSetLayout);
    device.destroyShaderModule(this->cShader);
    //The DescriptorSet does not need to be destroyed, It is managed by DescriptorPool.

    std::cout << std::endl
              << "destroyed everything successfully in task" << std::endl;
}
