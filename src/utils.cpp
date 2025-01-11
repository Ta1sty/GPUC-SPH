#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "utils.h"
#include <stb_image_write.h>

AppResources dontUse = AppResources();

AppResources &resources = dontUse;

std::vector<char> readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        std::string error = "failed to open file: " + filename;
        throw std::runtime_error(error);
    }
    size_t fileSize = (size_t)file.tellg();

    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    std::cout << "read " << buffer.size() << " bytes of data in file " << filename << std::endl;
    return buffer;
}

std::string formatSize(uint64_t size)
{
    std::ostringstream oss;
    if (size < 1024)
    {
        oss << size << " B";
    }
    else if (size < 1024 * 1024)
    {
        oss << size / 1024.f << " KB";
    }
    else if (size < 1024 * 1024 * 1024)
    {
        oss << size / (1024.0f * 1024.0f) << " MB";
    }
    else
    {
        oss << size / (1024.0f * 1024.0f * 1024.0f) << " GB";
    }
    return oss.str();
}

void writeFloatJpg(const std::string name, const std::vector<float> &inData, const int w, const int h)
{
    std::vector<uint8_t> refINT(inData.size());
    for (int i = 0; i < inData.size(); i++)
        refINT[i] = static_cast<uint8_t>(inData[i] * 255);
    stbi_write_jpg(name.c_str(), w, h, 1, refINT.data(), 100);
}
uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice &pdevice)
{
    vk::PhysicalDeviceMemoryProperties memProperties = pdevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


Buffer createBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                  const vk::DeviceSize &size, vk::BufferUsageFlags usage,
                  vk::MemoryPropertyFlags properties, std::string name)
{
    vk::BufferCreateInfo inBufferInfo({}, size, usage);
    vk::Buffer buffer = device.createBuffer(inBufferInfo);
    setObjectName(device, buffer, name);

    vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo allocInfo(memReq.size,
                                     findMemoryType(memReq.memoryTypeBits, properties, pDevice));

    vk::DeviceMemory bufferMemory = device.allocateMemory(allocInfo);
    device.bindBufferMemory(buffer, bufferMemory, 0U);

    return {buffer, bufferMemory};
}

void copyBuffer(vk::Device &device, vk::Queue &q, vk::CommandPool &commandPool,
                const vk::Buffer &srcBuffer, const vk::Buffer &dstBuffer, vk::DeviceSize byteSize)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    vk::BufferCopy copyRegion(0ULL, 0ULL, byteSize);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, q, commandPool, commandBuffer);
}

void createImage(vk::PhysicalDevice &pDevice, vk::Device &device, vk::ImageCreateInfo createInfo,vk::MemoryPropertyFlags properties,
                    std::string name, vk::Image &image, vk::DeviceMemory &imageMemory)
{
    image = device.createImage(createInfo);
    setObjectName(device, image, name);
    auto memReq = device.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo allocInfo(memReq.size, findMemoryType(memReq.memoryTypeBits, properties, pDevice));
    imageMemory = device.allocateMemory(allocInfo);
    device.bindImageMemory(image, imageMemory, 0U);
}


vk::CommandBuffer beginSingleTimeCommands(vk::Device &device, vk::CommandPool &commandPool)
{
    vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);

    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(vk::Device &device, vk::Queue &q,
                           vk::CommandPool &commandPool, vk::CommandBuffer &commandBuffer)
{
    commandBuffer.end();
    vk::SubmitInfo submitInfo(0U, nullptr, nullptr, 1U, &commandBuffer);
    q.submit({submitInfo}, nullptr);
    q.waitIdle();
    device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void ownershipTransfer(vk::Device &device, vk::CommandPool &srcCommandPool, vk::Queue &srcQueue, uint32_t srcQueueFamilyIndex, vk::CommandPool &dstCommandPool, vk::Queue &dstQueue, uint32_t dstQueueFamilyIndex, vk::Image &image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    {
        vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, srcCommandPool);

        vk::ImageMemoryBarrier barrier(
            vk::AccessFlagBits::eNoneKHR, vk::AccessFlagBits::eNoneKHR,
            oldLayout, newLayout,
            srcQueueFamilyIndex, dstQueueFamilyIndex,
            image,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,0,1,0,1));

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;
        
        using psf = vk::PipelineStageFlagBits;
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal ) {
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

            sourceStage = psf::eTopOfPipe;
            destinationStage = psf::eBottomOfPipe;
        } else if ( oldLayout == vk::ImageLayout::eTransferDstOptimal && 
                    newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = {};

            sourceStage = psf::eTransfer;
            destinationStage = psf::eBottomOfPipe;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        } 
        
        commandBuffer.pipelineBarrier(
            sourceStage, destinationStage,
            vk::DependencyFlagBits::eByRegion,
            0,nullptr,
            0,nullptr,
            1,&barrier
        );

        endSingleTimeCommands(device, srcQueue, srcCommandPool, commandBuffer);
    }

    {
        vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, dstCommandPool);

        vk::ImageMemoryBarrier barrier(
            vk::AccessFlagBits::eNoneKHR, vk::AccessFlagBits::eNoneKHR,
            oldLayout, newLayout,
            srcQueueFamilyIndex, dstQueueFamilyIndex,
            image,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,0,1,0,1));

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;
        
        using psf = vk::PipelineStageFlagBits;
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal ) {
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

            sourceStage = psf::eTopOfPipe;
            destinationStage = psf::eTransfer;
        } else if ( oldLayout == vk::ImageLayout::eTransferDstOptimal && 
                    newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = psf::eTopOfPipe;
            destinationStage = psf::eComputeShader;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        } 
        
        commandBuffer.pipelineBarrier(
            sourceStage, destinationStage,
            vk::DependencyFlagBits::eByRegion,
            0,nullptr,
            0,nullptr,
            1,&barrier
        );

        endSingleTimeCommands(device, dstQueue, dstCommandPool, commandBuffer);
    }
}

void transitionImageLayout(vk::Device &device, vk::CommandPool &pool, vk::Queue &queue, vk::Image &image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, pool);

    vk::ImageMemoryBarrier barrier(
        vk::AccessFlagBits::eNoneKHR, vk::AccessFlagBits::eNoneKHR,
        oldLayout, newLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,0,1,0,1));

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    
    using psf = vk::PipelineStageFlagBits;
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal ) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        sourceStage = psf::eTopOfPipe;
        destinationStage = psf::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferSrcOptimal) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

        sourceStage = psf::eTopOfPipe;
        destinationStage = psf::eTransfer;
    }else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = psf::eTransfer;
        destinationStage = psf::eComputeShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral){
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        sourceStage = psf::eTopOfPipe;
        destinationStage = psf::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

        sourceStage = psf::eTransfer;
        destinationStage = psf::eColorAttachmentOutput;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

        sourceStage = psf::eBottomOfPipe;
        destinationStage = psf::eColorAttachmentOutput;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    } 

    commandBuffer.pipelineBarrier(
        sourceStage, destinationStage,
        vk::DependencyFlagBits::eByRegion,
        0,nullptr,
        0,nullptr,
        1,&barrier
    );

    endSingleTimeCommands(device, queue, pool, commandBuffer);
}

void copyBufferToImage(vk::Device &device, vk::CommandPool &pool, vk::Queue &queue, vk::Buffer &buffer, vk::Image &image, uint32_t width, uint32_t height, uint32_t depth) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, pool);

    vk::BufferImageCopy region( 0,0,0,
                                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor,0,0,1),
                                vk::Offset3D(0,0,0), vk::Extent3D(width,height,depth));
    commandBuffer.copyBufferToImage(buffer,image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
    
    endSingleTimeCommands(device, queue, pool, commandBuffer);
}

Buffer createDeviceLocalBuffer(const std::string &name, vk::DeviceSize size, vk::BufferUsageFlags additionalUsageBits) {
    return createBuffer(
            resources.pDevice,
            resources.device,
            size,
            {additionalUsageBits | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst},
            {vk::MemoryPropertyFlagBits::eDeviceLocal},
            name
    );
}

void computeBarrier(vk::CommandBuffer &cmd) {
    cmd.pipelineBarrier(
            {vk::PipelineStageFlagBits::eComputeShader},
            {vk::PipelineStageFlagBits::eComputeShader},
            {},
            {vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead)},
            {},
            {}
    );
}
