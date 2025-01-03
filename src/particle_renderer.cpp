#include "particle_renderer.h"
#include "helper.h"
#include "simulation.h"

vk::Image ParticleRenderer::getImage() {
    return colorAttachment;
}

ParticleRenderer::ParticleRenderer(const SimulationParameters &simulationParameters) {
    vk::ImageCreateInfo imageInfo(
            {},
            vk::ImageType::e2D,
            resources.surfaceFormat.format,
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
            resources.surfaceFormat.format,
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
            resources.surfaceFormat.format,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::AttachmentDescription colorAttachmentDescription {
            {},
            resources.surfaceFormat.format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentReference colorAttachmentReference {
            0, vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::SubpassDescription particleSubpass {
            {},
            vk::PipelineBindPoint::eGraphics,
            {},
            colorAttachmentReference,
            {},
            nullptr,
            {}
    };

    vk::SubpassDependency externalDependency {
            VK_SUBPASS_EXTERNAL,
            0,
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::AccessFlagBits::eColorAttachmentWrite},
            {vk::AccessFlagBits::eColorAttachmentWrite}
    };

    vk::RenderPassCreateInfo renderPassCI {
            {},
            1U,
            &colorAttachmentDescription,
            1U,
            &particleSubpass,
            1U,
            &externalDependency
    };

    renderPass = resources.device.createRenderPass(renderPassCI);

    framebuffer = resources.device.createFramebuffer({
            {}, renderPass, 1, &colorAttachmentView, imageInfo.extent.width, imageInfo.extent.height, imageInfo.extent.depth
    });

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.emplace_back(
        0,
        vk::DescriptorType::eStorageBuffer,
        1U,
        vk::ShaderStageFlagBits::eGeometry
    );

    vk::DescriptorSetLayout descriptorSetLayout;
    Cmn::createDescriptorSetLayout(resources.device, bindings, descriptorSetLayout);

    //vk::PushConstantRange pcr {};

    vk::PipelineLayoutCreateInfo pipelineLayoutCI {
        vk::PipelineLayoutCreateFlags(),
        1U,
        &descriptorSetLayout,
        0U,
        nullptr
    };

    vk::PipelineLayout pipelineLayout = resources.device.createPipelineLayout(pipelineLayoutCI);

    vk::ShaderModule particleGeometrySM, particleVertexSM, particleFragmentSM;

    Cmn::createShader(resources.device, particleVertexSM, shaderPath("particle2d.vert"));
    Cmn::createShader(resources.device, particleGeometrySM, shaderPath("particle2d.geom"));
    Cmn::createShader(resources.device, particleFragmentSM, shaderPath("particle2d.frag"));

    // essentially copy pasted from project.h
    // TODO refactor and reuse for future pipeline initializations
    vk::PipelineShaderStageCreateInfo shaderStageCI[] {
            {{}, vk::ShaderStageFlagBits::eVertex, particleVertexSM, "main", nullptr},
            {{}, vk::ShaderStageFlagBits::eGeometry, particleGeometrySM, "main", nullptr},
            {{}, vk::ShaderStageFlagBits::eFragment, particleFragmentSM, "main", nullptr},
    };

    vk::VertexInputBindingDescription vertexInputBindings[] {
            { 0, 2 * 4, vk::VertexInputRate::eVertex }
    };

    vk::VertexInputAttributeDescription vertexInputAttributeDescriptions[] {
            { 0, 0, vk::Format::eR32G32Sfloat, 0 }
    };
//
    // Vertex input
    vk::PipelineVertexInputStateCreateInfo vertexInputSCI {
            {},
            1, // Vertex binding description  count
            vertexInputBindings, // List of Vertex Binding Descriptions (data spacing/stride information)
            1, // Vertex attribute description count
            vertexInputAttributeDescriptions // List of Vertex Attribute Descriptions (data format and where to bind to/from)
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblySCI {
            {},
            vk::PrimitiveTopology::ePointList,
            false,
    };

    // Viewport & Scissor
    vk::Viewport viewport = {
            0.f, // x start coordinate
            (float)resources.extent.height, // y start coordinate
            (float)resources.extent.width, // Width of viewport
            -(float)resources.extent.height, // Height of viewport
            0.f, // Min framebuffer depth,
            1.f // Max framebuffer depth
    };
    vk::Rect2D scissor = {
            {0, 0}, // Offset to use region from
            resources.extent // Extent to describe region to use, starting at offset
    };

    vk::PipelineViewportStateCreateInfo viewportSCI = {
            {},
            1, // Viewport count
            &viewport, // Viewport used
            1, // Scissor count
            &scissor // Scissor used
    };

    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizationSCI = {
            {},
            false, // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
            false,
            // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
            vk::PolygonMode::eFill, // How to handle filling points between vertices
            vk::CullModeFlagBits::eFront, // Which face of a tri to cull
            vk::FrontFace::eCounterClockwise, // Winding to determine which side is front
            false, // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
            0.f,
            0.f,
            0.f,
            1.f // How thick lines should be when drawn
    };

    vk::PipelineMultisampleStateCreateInfo multisampleSCI = {
            {},
            vk::SampleCountFlagBits::e1, // Number of samples to use per fragment
            false, // Enable multisample shading or not
            0.f,
            nullptr,
            false,
            false
    };
    // Depth stencil creation
    vk::PipelineDepthStencilStateCreateInfo depthStencilSCI = {
            {}, true, false, vk::CompareOp::eLess, false, false, {}, {}, 0.f, 0.f
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
        true,
        vk::BlendFactor::eOneMinusDstAlpha,
        vk::BlendFactor::eDstAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendSCI {
            {},
            false,
            {},
            1,
            &colorBlendAttachmentState,
            {}
    };

    vk::GraphicsPipelineCreateInfo particlePipelineCI {
            {},
            3,
            shaderStageCI,
            &vertexInputSCI,
            &inputAssemblySCI,
            nullptr,
            &viewportSCI,
            &rasterizationSCI,
            &multisampleSCI,
            &depthStencilSCI,
            &colorBlendSCI,
            nullptr,
            pipelineLayout,
            renderPass,
            0, // subpass
            {},
            0
    };

    auto pipelines = resources.device.createGraphicsPipelines(VK_NULL_HANDLE, { particlePipelineCI });
    if (pipelines.result != vk::Result::eSuccess)
        throw std::runtime_error("Pipeline creation failed");

    particlePipeline = pipelines.value[0];

    // need to create a dedicated buffer for vertex input, as this cannot be done from a storage buffer
    // this means we also need to copy the particle positions every invocation
    createBuffer(
            resources.pDevice,
            resources.device,
            simulationParameters.numParticles * 2 * 4,
            { vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst },
            { vk::MemoryPropertyFlagBits::eDeviceLocal },
            "vertex-input-particles",
            vertexInput,
            vertexInputMemory
    );

    commandBuffer = resources.device.allocateCommandBuffers(
                { resources.graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1U }
            )[0];
}

vk::CommandBuffer ParticleRenderer::run(const SimulationState &simulationState) {
    // image must be in eColorAttachmentOptimal after the command buffer executed!

    commandBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    vk::BufferCopy copyRegions { 0, 0, simulationState.coordinateBufferSize };
    commandBuffer.copyBuffer(simulationState.particleCoordinateBuffer, vertexInput, 1, &copyRegions);

    vk::ClearValue clearValue;
    clearValue.color.uint32 = {{ 0, 0, 0, 0 }};
    commandBuffer.beginRenderPass(
            { renderPass, framebuffer, {{ 0, 0}, resources.extent }, 1, &clearValue},
            vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, particlePipeline);
    uint64_t offsets[] = { 0UL };
    commandBuffer.bindVertexBuffers(0, 1, &vertexInput, offsets);
    commandBuffer.draw(simulationState.parameters.numParticles, 1, 0, 0);
    commandBuffer.endRenderPass();

    commandBuffer.end();

    return commandBuffer;
}
