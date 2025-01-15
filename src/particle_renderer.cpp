#include "particle_renderer.h"
#include "helper.h"
#include "simulation.h"

struct GraphicsPipelineBuilder {
    vk::Viewport viewport;
    vk::Rect2D scissor;
    vk::PipelineViewportStateCreateInfo viewportSCI;
    vk::PipelineRasterizationStateCreateInfo rasterizationSCI;
    vk::PipelineMultisampleStateCreateInfo multisampleSCI;
    vk::PipelineDepthStencilStateCreateInfo depthStencilSCI;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCIs;
    std::vector<vk::VertexInputBindingDescription> vertexInputBindings;
    std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
    vk::PipelineVertexInputStateCreateInfo vertexInputSci;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblySCI;
    std::vector<vk::PipelineColorBlendAttachmentState> pipelineColorBlendAttachmentStates;
    vk::PipelineColorBlendStateCreateInfo colorBlendSCI;
    vk::PipelineLayout pipelineLayout;
    vk::RenderPass renderPass;
    uint32_t subpass;
    std::vector<vk::ShaderModule> shaderModules;

    GraphicsPipelineBuilder(GraphicsPipelineBuilder &&obj) = default;

    explicit GraphicsPipelineBuilder(const std::initializer_list<std::pair<vk::ShaderStageFlagBits, const char *>> &shaders, const vk::PipelineLayout &pipelineLayout, const vk::RenderPass &renderPass, const uint32_t subpass) {
        for (const auto &[stage, file]: shaders) {
            vk::ShaderModule sm;
            Cmn::createShader(resources.device, sm, shaderPath(file));
            shaderModules.push_back(sm);
            shaderStageCIs.push_back({{}, stage, sm, "main", nullptr});
        }

        viewport = vk::Viewport {
                0.f,                             // x start coordinate
                (float) resources.extent.height, // y start coordinate
                (float) resources.extent.width,  // Width of viewport
                -(float) resources.extent.height,// Height of viewport
                0.f,                             // Min framebuffer depth,
                1.f                              // Max framebuffer depth
        };
        scissor = vk::Rect2D {
                {0, 0},         // Offset to use region from
                resources.extent// Extent to describe region to use, starting at offset
        };

        viewportSCI = vk::PipelineViewportStateCreateInfo {
                {},
                1,        // Viewport count
                &viewport,// Viewport used
                1,        // Scissor count
                &scissor  // Scissor used
        };

        // Rasterizer
        rasterizationSCI = vk::PipelineRasterizationStateCreateInfo {
                {},
                false,// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
                false,
                // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
                vk::PolygonMode::eFill,          // How to handle filling points between vertices
                vk::CullModeFlagBits::eBack,     // Which face of a tri to cull
                vk::FrontFace::eCounterClockwise,// Winding to determine which side is front
                false,                           // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
                0.f,
                0.f,
                0.f,
                1.f// How thick lines should be when drawn
        };

        multisampleSCI = vk::PipelineMultisampleStateCreateInfo {
                {},
                vk::SampleCountFlagBits::e1,// Number of samples to use per fragment
                false,                      // Enable multisample shading or not
                0.f,
                nullptr,
                false,
                false};

        // Depth stencil creation
        depthStencilSCI = vk::PipelineDepthStencilStateCreateInfo {
                {},
                vk::False
                //            {}, vk::True, vk::False, vk::CompareOp::eAlways, vk::False, vk::False, {}, {}, 0.0f, 0.0f
                //            {}, true, false, vk::CompareOp::eLess, false, false, {}, {}, 0.f, 0.f
        };
        colorBlendSCI = vk::PipelineColorBlendStateCreateInfo {
                {},
                false,
                {},
                0,
                nullptr,
                {}};
        pipelineColorBlendAttachmentStates.emplace_back(vk::PipelineColorBlendAttachmentState {
                true,
                vk::BlendFactor::eSrcAlpha,
                vk::BlendFactor::eOneMinusSrcAlpha,
                vk::BlendOp::eAdd,
                vk::BlendFactor::eOne,
                vk::BlendFactor::eZero,
                vk::BlendOp::eAdd,
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA});

        vertexInputSci = vk::PipelineVertexInputStateCreateInfo {
                {},
                0,
                nullptr,
                0,
                nullptr};
        vertexInputBindings.emplace_back(vk::VertexInputBindingDescription {
                0, 2 * 4, vk::VertexInputRate::eVertex});
        vertexInputAttributeDescriptions.emplace_back(vk::VertexInputAttributeDescription {
                0, 0, vk::Format::eR32G32Sfloat, 0});

        inputAssemblySCI = vk::PipelineInputAssemblyStateCreateInfo {
                {},
                vk::PrimitiveTopology::eTriangleList,
                false};

        this->pipelineLayout = pipelineLayout;
        this->renderPass = renderPass;
        this->subpass = subpass;
    }

    ~GraphicsPipelineBuilder() {
        for (auto &sm: shaderModules)
            resources.device.destroyShaderModule(sm);
    }

private:
    void finalize() {
        if (vertexInputSci.vertexBindingDescriptionCount == 0 || vertexInputSci.pVertexBindingDescriptions == nullptr) {
            vertexInputSci.vertexBindingDescriptionCount = vertexInputBindings.size();
            vertexInputSci.pVertexBindingDescriptions = vertexInputBindings.data();
        }

        if (vertexInputSci.vertexAttributeDescriptionCount == 0 || vertexInputSci.pVertexAttributeDescriptions == nullptr) {
            vertexInputSci.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
            vertexInputSci.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
        }

        if (colorBlendSCI.attachmentCount == 0 || colorBlendSCI.pAttachments == nullptr) {
            colorBlendSCI.attachmentCount = pipelineColorBlendAttachmentStates.size();
            colorBlendSCI.pAttachments = pipelineColorBlendAttachmentStates.data();
        }
    }

public:
    static auto createPipelines(std::vector<GraphicsPipelineBuilder> &builders) {
        std::vector<vk::GraphicsPipelineCreateInfo> pipelineCIs;
        for (auto &builder: builders) {
            builder.finalize();
            pipelineCIs.push_back({{},
                                   static_cast<uint32_t>(builder.shaderStageCIs.size()),
                                   builder.shaderStageCIs.data(),
                                   &builder.vertexInputSci,
                                   &builder.inputAssemblySCI,
                                   nullptr,// tesselation
                                   &builder.viewportSCI,
                                   &builder.rasterizationSCI,
                                   &builder.multisampleSCI,
                                   &builder.depthStencilSCI,
                                   &builder.colorBlendSCI,
                                   nullptr,// dynamic state
                                   builder.pipelineLayout,
                                   builder.renderPass,
                                   builder.subpass,
                                   {},
                                   0});
        }

        return resources.device.createGraphicsPipelines(nullptr, pipelineCIs);
    }
};

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
            vk::ImageLayout::eUndefined);

    createImage(
            resources.pDevice,
            resources.device,
            imageInfo,
            {vk::MemoryPropertyFlagBits::eDeviceLocal},
            "render-color-attachment",
            colorAttachment,
            colorAttachmentMemory);

    vk::ImageViewCreateInfo viewInfo(
            {},
            colorAttachment,
            vk::ImageViewType::e2D,
            resources.surfaceFormat.format,
            {},
            {{vk::ImageAspectFlagBits::eColor},
             0,
             1,
             0,
             1});
    colorAttachmentView = resources.device.createImageView(viewInfo);

    // can be removed later - layout transitions for the color attachment should be handled by render-pass
    transitionImageLayout(
            resources.device,
            resources.graphicsCommandPool,
            resources.graphicsQueue,
            colorAttachment,
            resources.surfaceFormat.format,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription colorAttachmentDescription {
            {},
            resources.surfaceFormat.format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal};

    vk::AttachmentReference colorAttachmentReference {
            0, vk::ImageLayout::eColorAttachmentOptimal};

    vk::SubpassDescription subpasses[] {
            {// background subpass
             {},
             vk::PipelineBindPoint::eGraphics,
             {},
             colorAttachmentReference,
             {},
             nullptr,
             {}},
            {// particle subpass
             {},
             vk::PipelineBindPoint::eGraphics,
             {},
             colorAttachmentReference,
             {},
             nullptr,
             {}}};

    vk::SubpassDependency dependencies[] {
            {// external dependency
             VK_SUBPASS_EXTERNAL,
             0,
             {vk::PipelineStageFlagBits::eColorAttachmentOutput},
             {vk::PipelineStageFlagBits::eColorAttachmentOutput},
             {vk::AccessFlagBits::eColorAttachmentWrite},
             {vk::AccessFlagBits::eColorAttachmentWrite}},
            {0,
             1,
             {vk::PipelineStageFlagBits::eColorAttachmentOutput},
             {vk::PipelineStageFlagBits::eColorAttachmentOutput},
             {vk::AccessFlagBits::eColorAttachmentWrite},
             {vk::AccessFlagBits::eColorAttachmentWrite}}};

    vk::RenderPassCreateInfo renderPassCI {
            {},
            1U,
            &colorAttachmentDescription,
            2U,
            subpasses,
            2U,
            dependencies};

    renderPass = resources.device.createRenderPass(renderPassCI);

    framebuffer = resources.device.createFramebuffer({{}, renderPass, 1, &colorAttachmentView, imageInfo.extent.width, imageInfo.extent.height, imageInfo.extent.depth});

    createColormapTexture(colormaps::viridis);
    createPipeline();

    // quad vertex buffer
    const std::vector<glm::vec2> quadVertices {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}};

    const std::vector<uint16_t> quadIndices {
            0, 1, 2, 2, 3, 0};

    quadVertexBuffer = createBuffer(resources.pDevice, resources.device, quadVertices.size() * sizeof(glm::vec2),
                                    {vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst},
                                    {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                    "quadVertexBuffer");

    quadIndexBuffer = createBuffer(resources.pDevice, resources.device, quadIndices.size() * sizeof(uint16_t),
                                   {vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst},
                                   {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                   "quadIndexBuffer");

    fillDeviceWithStagingBuffer(quadVertexBuffer, quadVertices);
    fillDeviceWithStagingBuffer(quadIndexBuffer, quadIndices);

    uniformBuffer = createBuffer(resources.pDevice, resources.device, sizeof(UniformBufferStruct),
                                 {vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst},
                                 {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                 "render2dUniformBuffer");

    const std::vector<UniformBufferStruct> uniformBufferVector {uniformBufferContent};
    fillDeviceWithStagingBuffer(uniformBuffer, uniformBufferVector);
}

vk::CommandBuffer ParticleRenderer::run(const SimulationState &simulationState, const RenderParameters &renderParameters) {
    UniformBufferStruct ub {
            simulationState.parameters.numParticles,
            static_cast<uint32_t>(renderParameters.backgroundField),
            renderParameters.particleRadius,
            simulationState.spatialRadius};

    if (!(ub == uniformBufferContent)) {
        uniformBufferContent = ub;
        const std::vector<UniformBufferStruct> uniformBufferVector {uniformBufferContent};
        fillDeviceWithStagingBuffer(uniformBuffer, uniformBufferVector);
    }

    if (commandBuffer == nullptr)
        updateCmd(simulationState);

    return commandBuffer;
}

ParticleRenderer::~ParticleRenderer() {
    resources.device.destroyFramebuffer(framebuffer);
    resources.device.destroyImageView(colorAttachmentView);
    resources.device.destroyImage(colorAttachment);
    resources.device.freeMemory(colorAttachmentMemory);

    resources.device.freeCommandBuffers(resources.graphicsCommandPool, commandBuffer);
    resources.device.destroyPipeline(particlePipeline);
    resources.device.destroyPipeline(backgroundPipeline);
    resources.device.destroyPipelineLayout(particlePipelineLayout);
    resources.device.destroyRenderPass(renderPass);

    resources.device.destroySampler(colormapSampler);
    resources.device.destroyImageView(colormapImageView);
    resources.device.destroyImage(colormapImage);
    resources.device.freeMemory(colormapImageMemory);

    resources.device.destroyDescriptorPool(descriptorPool);
    resources.device.destroyDescriptorSetLayout(descriptorSetLayout);
}

void ParticleRenderer::createPipeline() {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.emplace_back(
            0,
            vk::DescriptorType::eStorageBuffer,
            1U,
            vk::ShaderStageFlagBits::eFragment);
    bindings.emplace_back(
            1,
            vk::DescriptorType::eCombinedImageSampler,
            1U,
            vk::ShaderStageFlagBits::eFragment,
            nullptr);
    bindings.emplace_back(
            2,
            vk::DescriptorType::eUniformBuffer,
            1U,
            vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eGeometry);

    bindings.emplace_back(// spatial-lookup
            3,
            vk::DescriptorType::eStorageBuffer,
            1U,
            vk::ShaderStageFlagBits::eFragment);

    bindings.emplace_back(// spatial-indices
            4,
            vk::DescriptorType::eStorageBuffer,
            1U,
            vk::ShaderStageFlagBits::eFragment);

    Cmn::createDescriptorSetLayout(resources.device, bindings, descriptorSetLayout);
    Cmn::createDescriptorPool(resources.device, bindings, descriptorPool);
    Cmn::allocateDescriptorSet(resources.device, descriptorSet, descriptorPool, descriptorSetLayout);

    vk::PushConstantRange pcr {
            vk::ShaderStageFlagBits::eAll, 0, sizeof(PushStruct)};

    vk::PipelineLayoutCreateInfo pipelineLayoutCI {
            vk::PipelineLayoutCreateFlags(),
            1U,
            &descriptorSetLayout,
            1U,
            &pcr};

    particlePipelineLayout = resources.device.createPipelineLayout(pipelineLayoutCI);

    std::vector<GraphicsPipelineBuilder> builders;
    builders.emplace_back(GraphicsPipelineBuilder {
            {{vk::ShaderStageFlagBits::eVertex, "background2d.vert"},
             {vk::ShaderStageFlagBits::eFragment, "background2d.frag"}},
            particlePipelineLayout,
            renderPass,
            0});

    builders.emplace_back(GraphicsPipelineBuilder {
            {{vk::ShaderStageFlagBits::eVertex, "particle2d.vert"},
             {vk::ShaderStageFlagBits::eGeometry, "particle2d.geom"},
             {vk::ShaderStageFlagBits::eFragment, "particle2d.frag"}},
            particlePipelineLayout,
            renderPass,
            1});
    builders[1].inputAssemblySCI.topology = vk::PrimitiveTopology::ePointList;

    auto pipelines = GraphicsPipelineBuilder::createPipelines(builders);
    if (pipelines.result != vk::Result::eSuccess)
        throw std::runtime_error("Pipeline creation failed");

    backgroundPipeline = pipelines.value[0];
    particlePipeline = pipelines.value[1];
}

void ParticleRenderer::createColormapTexture(const std::vector<colormaps::RGB_F32> &colormap) {
    auto imageFormat = vk::Format::eR8G8B8A8Unorm;
    vk::Extent3D imageExtent = {static_cast<uint32_t>(colormap.size()), 1, 1};

    vk::ImageCreateInfo imageCI {
            {},
            vk::ImageType::e1D,
            imageFormat,
            imageExtent,
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            {vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst},
            vk::SharingMode::eExclusive,
            1,
            &resources.gQ,
            vk::ImageLayout::eUndefined,
    };

    createImage(
            resources.pDevice,
            resources.device,
            imageCI,
            {vk::MemoryPropertyFlagBits::eDeviceLocal},
            "colormapTexture",
            colormapImage,
            colormapImageMemory);

    vk::ImageViewCreateInfo viewCI {
            {},
            colormapImage,
            vk::ImageViewType::e1D,
            imageFormat,
            {},
            {{vk::ImageAspectFlagBits::eColor}, 0, 1, 0, 1}};

    colormapImageView = resources.device.createImageView(viewCI);

    struct RGBA_int8 {
        uint8_t r, g, b, a;
    };
    std::vector<RGBA_int8> converted {colormap.size(), {0, 0, 0}};
    for (size_t i = 0; i < colormap.size(); i++) {
        auto &c = colormap[i];
        converted[i] = {
                static_cast<uint8_t>(c.r * 256.0f),
                static_cast<uint8_t>(c.g * 256.0f),
                static_cast<uint8_t>(c.b * 256.0f),
                255};
    }

    fillImageWithStagingBuffer(colormapImage, vk::ImageLayout::eShaderReadOnlyOptimal, imageExtent, converted);

    vk::SamplerCreateInfo samplerCI {
            {},
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            {},
            vk::False,
            {},
            vk::False,
            {},
            0,
            0,
            vk::BorderColor::eFloatOpaqueBlack,
            vk::False};

    colormapSampler = resources.device.createSampler(samplerCI);
}

void ParticleRenderer::updateDescriptorSets(const SimulationState &simulationState) {
    Cmn::bindBuffers(resources.device, simulationState.particleCoordinateBuffer.buf, descriptorSet, 0);
    Cmn::bindCombinedImageSampler(resources.device, colormapImageView, colormapSampler, descriptorSet, 1);
    Cmn::bindBuffers(resources.device, uniformBuffer.buf, descriptorSet, 2, vk::DescriptorType::eUniformBuffer);
    Cmn::bindBuffers(resources.device, simulationState.spatialLookup.buf, descriptorSet, 3);
    Cmn::bindBuffers(resources.device, simulationState.spatialIndices.buf, descriptorSet, 4);
}

void ParticleRenderer::updateCmd(const SimulationState &simulationState) {
    if (commandBuffer == nullptr)
        commandBuffer = resources.device.allocateCommandBuffers(
                {resources.graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1U})[0];
    else
        commandBuffer.reset();

    // image must be in eColorAttachmentOptimal after the command buffer executed!
    updateDescriptorSets(simulationState);
    pushStruct.width = resources.extent.width;
    pushStruct.height = resources.extent.height;

    // map [0, 1]^2 into viewport
    pushStruct.mvp = simulationState.camera->viewProjectionMatrix();

    commandBuffer.begin(vk::CommandBufferBeginInfo {});
    uint64_t offsets[] = {0UL};

    vk::ClearValue clearValue;
    clearValue.color.uint32 = {{0, 0, 0, 0}};
    commandBuffer.beginRenderPass(
            {renderPass, framebuffer, {{0, 0}, resources.extent}, 1, &clearValue},
            vk::SubpassContents::eInline);
    /* ========== Background Subpass ========== */
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, backgroundPipeline);
    commandBuffer.bindVertexBuffers(0, 1, &quadVertexBuffer.buf, offsets);
    commandBuffer.bindIndexBuffer(quadIndexBuffer.buf, 0UL, vk::IndexType::eUint16);
    commandBuffer.pushConstants(particlePipelineLayout, vk::ShaderStageFlagBits::eAll, 0, sizeof(PushStruct), &pushStruct);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, particlePipelineLayout, 0, 1, &descriptorSet,
                                     0, nullptr);
    commandBuffer.drawIndexed(6, 1, 0, 0, 0);// draw quad

    /* ========== Particle Subpass ========== */
    commandBuffer.nextSubpass(vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, particlePipeline);
    commandBuffer.bindVertexBuffers(0, 1, &simulationState.particleCoordinateBuffer.buf, offsets);

    commandBuffer.pushConstants(particlePipelineLayout, vk::ShaderStageFlagBits::eAll, 0, sizeof(PushStruct), &pushStruct);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, particlePipelineLayout, 0, 1, &descriptorSet,
                                     0, nullptr);
    commandBuffer.draw(simulationState.parameters.numParticles, 1, 0, 0);

    /* ====================================== */
    commandBuffer.endRenderPass();
    commandBuffer.end();
}
