#include "debug_image.h"

DebugImage::DebugImage(std::string name) {

    vk::ImageCreateInfo imageInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {resources.extent.width, resources.extent.height, 1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            {vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst},
            vk::SharingMode::eExclusive,
            1,
            &resources.gQ,
            vk::ImageLayout::eUndefined
    );

    createImage(resources.pDevice, resources.device, imageInfo, {vk::MemoryPropertyFlagBits::eDeviceLocal}, std::move(name), image, imageMemory);

    vk::ImageViewCreateInfo viewInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {},
            {{vk::ImageAspectFlagBits::eColor},
             0, 1, 0, 1}
    );
    view = resources.device.createImageView(viewInfo);

    descriptorInfo = vk::DescriptorImageInfo(nullptr, view, vk::ImageLayout::eColorAttachmentOptimal);

    transitionImageLayout(
            resources.device,
            resources.graphicsCommandPool,
            resources.graphicsQueue,
            image,
            resources.surfaceFormat.format,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
    );
}

DebugImage::~DebugImage() {
    resources.device.destroyImageView(view);
    resources.device.destroyImage(image);
    resources.device.freeMemory(imageMemory);
}

void DebugImage::clear(vk::CommandBuffer cmd, std::array<float,4> color) {
    vk::ImageSubresourceRange resource(vk::ImageAspectFlagBits::eColor, 0, 1,0,1);
    cmd.clearColorImage(image, vk::ImageLayout::eGeneral, color, resource);
}

vk::DescriptorSetLayoutBinding DebugImage::getLayout(uint32_t binding) {
    return {binding, vk::DescriptorType::eStorageImage, 1, {vk::ShaderStageFlagBits::eAll}, nullptr};
}

vk::WriteDescriptorSet DebugImage::getWrite(vk::DescriptorSet set, uint32_t binding) {
    return {set, binding, 0,vk::DescriptorType::eStorageImage, descriptorInfo };
}


