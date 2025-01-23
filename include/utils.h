#pragma once

#include <cmath>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>

#include "GLFW/glfw3.h"
#include "helper.h"
#include <vulkan/vulkan.hpp>

enum Query {
    ResetBegin = 0,
    ResetEnd = 1,
    PhysicsBegin = 2,
    PhysicsEnd = 3,
    LookupBegin = 4,
    LookupEnd = 5,
    RenderBegin = 6,
    RenderEnd = 7,
    CopyBegin = 8,
    CopyEnd = 9,
    UiBegin = 10,
    UiEnd = 11,
    COUNT = 12,
};

struct AppResources {
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT dbgUtilsMgr;
    vk::PhysicalDevice pDevice;
    float timestampPeriod;

    vk::Device device;
    vk::Queue graphicsQueue, computeQueue, transferQueue;
    uint32_t gQ, cQ, tQ;
    vk::CommandPool graphicsCommandPool, computeCommandPool, transferCommandPool;
    vk::QueryPool queryPool;

    GLFWwindow *window;
    vk::Extent2D extent;
    vk::SurfaceKHR surface;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;

    void destroy();
};

struct AppResources;

extern AppResources &resources;

#define CAST(a) static_cast<uint32_t>(a.size())

struct Buffer {
    Buffer(Buffer &other) = delete;            // copy disabled
    Buffer &operator=(const Buffer &) = delete;// Copy assignment disabled

    Buffer() : buf(nullptr), mem(nullptr) {}
    Buffer(vk::Buffer buf, vk::DeviceMemory mem) : buf(buf), mem(mem) {}

    Buffer(Buffer &&other) noexcept : buf(other.buf), mem(other.mem) {
        other.buf = nullptr;
        other.mem = nullptr;
    }

    Buffer &operator=(Buffer &&other) noexcept// move is allowed
    {
        if (this != &other) {
            // Clean up existing resources
            if (buf != nullptr) {
                resources.device.destroyBuffer(buf);
            }
            if (mem != nullptr) {
                resources.device.freeMemory(mem);
            }
            buf = other.buf;
            mem = other.mem;
            other.buf = nullptr;
            other.mem = nullptr;
        }
        return *this;
    }

    vk::Buffer buf;
    vk::DeviceMemory mem;

    ~Buffer() {
        if (nullptr != buf) {
            resources.device.destroyBuffer(buf);
        }
        if (nullptr != mem) {
            resources.device.freeMemory(mem);
        }
    }
};

std::vector<char> readFile(const std::string &filename);
std::string formatSize(uint64_t size);
uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice &pdevice);
void writeTimestamp(vk::CommandBuffer cmd, Query value);
void ownershipTransfer(vk::Device &device, vk::CommandPool &srcCommandPool, vk::Queue &srcQueue, uint32_t srcQueueFamilyIndex, vk::CommandPool &dstCommandPool, vk::Queue &dstQueue, uint32_t dstQueueFamilyIndex, vk::Image &image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
void transitionImageLayout(vk::Device &device, vk::CommandPool &pool, vk::Queue &queue, vk::Image &image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
void copyBufferToImage(vk::Device &device, vk::CommandPool &pool, vk::Queue &queue, vk::Buffer &buffer, vk::Image &image, uint32_t width, uint32_t height, uint32_t depth);

Buffer createDeviceLocalBuffer(const std::string &name, vk::DeviceSize size, vk::BufferUsageFlags additionalUsageBits = {});

Buffer createBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                    const vk::DeviceSize &size, vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties, std::string name);

void createImage(vk::PhysicalDevice &pDevice, vk::Device &device, vk::ImageCreateInfo createInfo, vk::MemoryPropertyFlags properties,
                 std::string name, vk::Image &image, vk::DeviceMemory &imageMemory);
void copyBuffer(vk::Device &device, vk::Queue &q, vk::CommandPool &commandPool,
                const vk::Buffer &srcBuffer, const vk::Buffer &dstBuffer, vk::DeviceSize byteSize);

vk::CommandBuffer beginSingleTimeCommands(vk::Device &device, vk::CommandPool &commandPool);
void endSingleTimeCommands(vk::Device &device, vk::Queue &q,
                           vk::CommandPool &commandPool, vk::CommandBuffer &commandBuffer);

Buffer addHostCoherentBuffer(vk::PhysicalDevice &pDevice, vk::Device &device, vk::DeviceSize size, std::string name);
Buffer addDeviceOnlyBuffer(vk::PhysicalDevice &pDevice, vk::Device &device, vk::DeviceSize size, std::string name);

void writeFloatJpg(const std::string name, const std::vector<float> &inData, const int w, const int h);

template<typename T>
void fillDeviceBuffer(vk::Device &device, vk::DeviceMemory &mem, const std::vector<T> &input) {
    void *data = device.mapMemory(mem, 0, input.size() * sizeof(T), vk::MemoryMapFlags());
    memcpy(data, input.data(), static_cast<size_t>(input.size() * sizeof(T)));
    device.unmapMemory(mem);
}

template<typename T>
void fillHostBuffer(vk::Device &device, vk::DeviceMemory &mem, std::vector<T> &output) {
    // copy memory from mem to output
    void *data = device.mapMemory(mem, 0, output.size() * sizeof(T), vk::MemoryMapFlags());
    memcpy(output.data(), data, static_cast<size_t>(output.size() * sizeof(T)));
    device.unmapMemory(mem);
}

template<typename T>
void fillImageWithStagingBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                                vk::CommandPool &commandPool, vk::Queue &q,
                                vk::Image &image, vk::ImageLayout targetLayout, const vk::Extent3D &extent,
                                const std::vector<T> &data) {
    vk::DeviceSize byteSize = data.size() * sizeof(T);

    vk::ImageSubresourceRange subresourceRange {{vk::ImageAspectFlagBits::eColor}, 0, 1, 0, 1};

    auto staging = createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, "staging");
    // V host -> staging V
    fillDeviceBuffer<T>(device, staging.mem, data);
    auto cb = beginSingleTimeCommands(device, commandPool);

    vk::ImageMemoryBarrier toTransferLayout {
            {},
            {},
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            {},
            {},
            image,
            subresourceRange};
    cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0,
                       nullptr, 1, &toTransferLayout);

    vk::BufferImageCopy bufferImageCopy {
            0,
            0,
            0,
            {{vk::ImageAspectFlagBits::eColor}, 0, 0, 1},
            {0, 0, 0},
            extent};
    cb.copyBufferToImage(staging.buf, image, vk::ImageLayout::eTransferDstOptimal, 1, &bufferImageCopy);

    vk::ImageMemoryBarrier toTargetLayout {
            {},
            {},
            vk::ImageLayout::eTransferDstOptimal,
            targetLayout,
            {},
            {},
            image,
            subresourceRange};
    cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0,
                       nullptr, 1, &toTargetLayout);

    endSingleTimeCommands(device, q, commandPool, cb);
}

template<typename T>
void fillImageWithStagingBuffer(vk::Image &image, vk::ImageLayout targetLayout, const vk::Extent3D &extent,
                                const std::vector<T> &data) {
    fillImageWithStagingBuffer(resources.pDevice, resources.device, resources.transferCommandPool,
                               resources.transferQueue, image, targetLayout, extent, data);
}

// C++ 17 allows using it like  this:
// fillDeviceWithStagingBuffer(arguments)
// instead of
// fillDeviceWithStagingBuffer<vectorType>(arguments)
template<typename T>
void fillDeviceWithStagingBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                                 vk::CommandPool &commandPool, vk::Queue &q,
                                 Buffer &b, const std::vector<T> &data) {
    // Buffer b requires the eTransferSrc bit
    // data (host) -> staging (device) -> Buffer b (device)

    vk::DeviceSize byteSize = data.size() * sizeof(T);

    auto staging = createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, "staging");
    // V host -> staging V
    fillDeviceBuffer<T>(device, staging.mem, data);
    // V staging -> buffer V
    copyBuffer(device, q, commandPool, staging.buf, b.buf, byteSize);
}
template<typename T>
void fillDeviceWithStagingBuffer(Buffer &b, const std::vector<T> &data) {
    fillDeviceWithStagingBuffer(resources.pDevice, resources.device, resources.transferCommandPool, resources.transferQueue, b, data);
}

template<typename T>
void fillHostWithStagingBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                               vk::CommandPool &commandPool, vk::Queue &q,
                               const Buffer &b, std::vector<T> &data) {
    // Buffer b requires the eTransferDst bit
    // Buffer b (device) -> staging (device) -> data (host)
    vk::DeviceSize byteSize = data.size() * sizeof(T);

    auto staging = createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferDst,
                                vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, "staging");
    // V buffer -> staging V
    copyBuffer(device, q, commandPool, b.buf, staging.buf, byteSize);
    // V staging -> host V
    fillHostBuffer<T>(device, staging.mem, data);
}

template<typename T>
void fillHostWithStagingBuffer(const Buffer &b, std::vector<T> &data) {
    fillHostWithStagingBuffer<T>(resources.pDevice, resources.device, resources.transferCommandPool, resources.transferQueue, b, data);
}

template<typename T>
void setObjectName(vk::Device &device, T handle, std::string name) {
#ifndef NDEBUG
    vk::DebugUtilsObjectNameInfoEXT infoEXT(handle.objectType, uint64_t(static_cast<typename T::CType>(handle)), name.c_str());
    device.setDebugUtilsObjectNameEXT(infoEXT);
#endif
}

void computeBarrier(vk::CommandBuffer &cmd);


uint32_t nextPowerOfTwo(uint32_t n);