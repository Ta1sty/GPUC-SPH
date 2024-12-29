#include "particle_renderer.h"

vk::CommandBuffer ParticleRenderer::run() {


    // image must be in eColorAttachmentOptimal after the command buffer executed!

    return nullptr;
}

vk::Image ParticleRenderer::getImage() {
    return colorAttachment;
}

ParticleRenderer::ParticleRenderer() {

    vk::ImageCreateInfo imageInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {resources.extent.width, resources.extent.height, 1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            {vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc},
            vk::SharingMode::eExclusive,
            1,
            &resources.gQ,
            vk::ImageLayout::eUndefined
    );

    createImage(
            resources.pDevice,
            resources.device,
            imageInfo,
            {vk::MemoryPropertyFlagBits::eDeviceLocal},
            "render-color-attachment",
            colorAttachment,
            colorAttachmentMemory
    );

    vk::ImageViewCreateInfo viewInfo(
            {},
            colorAttachment,
            vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {},
            {{vk::ImageAspectFlagBits::eColor},
             0, 1, 0, 1}
    );
    colorAttachmentView = resources.device.createImageView(viewInfo);

    // can be removed later - layout transitions for the color attachment should be handled by render-pass
    transitionImageLayout(
            resources.device,
            resources.graphicsCommandPool,
            resources.graphicsQueue,
            colorAttachment,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
    );

}
