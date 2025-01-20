#include "particle_renderer.h"
#include "helper.h"

/**
 * Convenience struct that initializes graphics pipeline parameters with sane values.
 * Create a graphics pipeline by using this struct and just overwriting any fields as you need.
 */
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

    explicit GraphicsPipelineBuilder(const std::initializer_list<std::pair<vk::ShaderStageFlagBits, const char *>> &shaders,
                                     const vk::PipelineLayout &pipelineLayout, const vk::RenderPass &renderPass, const uint32_t subpass) {
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

    vk::GraphicsPipelineCreateInfo buildCreateInfo() {
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


        return {{},
                static_cast<uint32_t>(shaderStageCIs.size()),
                shaderStageCIs.data(),
                &vertexInputSci,
                &inputAssemblySCI,
                nullptr,// tesselation
                &viewportSCI,
                &rasterizationSCI,
                &multisampleSCI,
                &depthStencilSCI,
                &colorBlendSCI,
                nullptr,// dynamic state
                pipelineLayout,
                renderPass,
                subpass,
                {},
                0};
    }

    static auto createPipelines(std::vector<GraphicsPipelineBuilder> &builders) {
        std::vector<vk::GraphicsPipelineCreateInfo> pipelineCIs;
        for (auto &builder: builders)
            pipelineCIs.push_back(builder.buildCreateInfo());

        auto pipelines = resources.device.createGraphicsPipelines(nullptr, pipelineCIs);
        if (pipelines.result != vk::Result::eSuccess)
            throw std::runtime_error("Pipeline creation failed");
        return pipelines.value;
    }
};

Texture::~Texture() {
    if (sampler)
        resources.device.destroySampler(sampler);
    if (view)
        resources.device.destroyImageView(view);
    if (image)
        resources.device.destroyImage(image);
    if (memory)
        resources.device.freeMemory(memory);
}

Texture Texture::createColormapTexture(const std::vector<colormaps::RGB_F32> &colormap) {
    Texture r;

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
            r.image,
            r.memory);

    vk::ImageViewCreateInfo viewCI {
            {},
            r.image,
            vk::ImageViewType::e1D,
            imageFormat,
            {},
            {{vk::ImageAspectFlagBits::eColor}, 0, 1, 0, 1}};

    r.view = resources.device.createImageView(viewCI);

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

    fillImageWithStagingBuffer(r.image, vk::ImageLayout::eShaderReadOnlyOptimal, imageExtent, converted);

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

    r.sampler = resources.device.createSampler(samplerCI);

    return std::move(r);
}

ParticleRenderer::ParticleRenderer() : imageSize(resources.extent.width, resources.extent.height, 1) {
    colormap = Texture::createColormapTexture(colormaps::viridis);

    // color attachment image
    {
        vk::ImageCreateInfo imageInfo(
                {},
                vk::ImageType::e2D,
                resources.surfaceFormat.format,
                imageSize,
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
    }

    // render pass/framebuffer
    {
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

        framebuffer = resources.device.createFramebuffer({{},
                                                          renderPass,
                                                          1,
                                                          &colorAttachmentView,
                                                          imageSize.width,
                                                          imageSize.height,
                                                          imageSize.depth});
    }

    uniformBuffer = createBuffer(resources.pDevice, resources.device, sizeof(UniformBufferStruct),
                                 {vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst},
                                 {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                 "render2dUniformBuffer");

    const std::vector<UniformBufferStruct> uniformBufferVector {uniformBufferContent};
    fillDeviceWithStagingBuffer(uniformBuffer, uniformBufferVector);

    background2DPipeline = std::make_unique<Background2DPipeline>(renderPass, 0, framebuffer, this);
    particleCirclePipeline = std::make_unique<ParticleCirclePipeline>(renderPass, 1, framebuffer, this);
}

ParticleRenderer::~ParticleRenderer() {
    resources.device.freeCommandBuffers(resources.graphicsCommandPool, commandBuffer);
    resources.device.destroyFramebuffer(framebuffer);
    resources.device.destroyRenderPass(renderPass);

    resources.device.destroyImageView(colorAttachmentView);
    resources.device.destroyImage(colorAttachment);
    resources.device.freeMemory(colorAttachmentMemory);
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

void ParticleRenderer::updateCmd(const SimulationState &simulationState) {
    if (commandBuffer == nullptr)
        commandBuffer = resources.device.allocateCommandBuffers(
                {resources.graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1U})[0];
    else
        commandBuffer.reset();


    commandBuffer.begin(vk::CommandBufferBeginInfo {});
    vk::ClearValue clearValue;
    clearValue.color.uint32 = {{0, 0, 0, 0}};
    commandBuffer.beginRenderPass(
            {renderPass, framebuffer, {{0, 0}, resources.extent}, 1, &clearValue},
            vk::SubpassContents::eInline);


    /* ========== Background Subpass ========== */
    switch (simulationState.parameters.type) {
        case SceneType::SPH_BOX_2D:
            background2DPipeline->draw(commandBuffer, simulationState);
            break;
        default:
            break;
    }

    /* ========== Particle Subpass ========== */
    commandBuffer.nextSubpass(vk::SubpassContents::eInline);
    particleCirclePipeline->draw(commandBuffer, simulationState);

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

vk::Image ParticleRenderer::getImage() {
    return colorAttachment;
}

ParticleCirclePipeline::ParticleCirclePipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, ParticleRenderer *_renderer) : renderer(_renderer) {
    descriptorPool.addStorage(0, 1, vk::ShaderStageFlagBits::eFragment);
    descriptorPool.addSampler(1, 1, vk::ShaderStageFlagBits::eFragment);
    descriptorPool.addUniform(2, 1, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eGeometry);
    descriptorPool.allocate();

    pipelineLayout = createPipelineLayout<PushStruct>(descriptorPool);

    std::vector<GraphicsPipelineBuilder> builders;
    builders.emplace_back(GraphicsPipelineBuilder {
            {{vk::ShaderStageFlagBits::eVertex, "particle2d.vert"},
             {vk::ShaderStageFlagBits::eGeometry, "particle2d.geom"},
             {vk::ShaderStageFlagBits::eFragment, "particle2d.frag"}},
            pipelineLayout,
            renderPass,
            subpass});
    builders[0].inputAssemblySCI.topology = vk::PrimitiveTopology::ePointList;
    builders.emplace_back(GraphicsPipelineBuilder {
            {{vk::ShaderStageFlagBits::eVertex, "particle3d.vert"},
             {vk::ShaderStageFlagBits::eGeometry, "particle2d.geom"},
             {vk::ShaderStageFlagBits::eFragment, "particle2d.frag"}},
            pipelineLayout,
            renderPass,
            subpass});
    builders[1].inputAssemblySCI.topology = vk::PrimitiveTopology::ePointList;
    builders[1].vertexInputBindings[0].stride = 4 * 4;
    builders[1].vertexInputAttributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;

    auto pipelines = GraphicsPipelineBuilder::createPipelines(builders);

    pipeline2d = pipelines[0];
    pipeline3d = pipelines[1];
}

ParticleCirclePipeline::~ParticleCirclePipeline() {
    resources.device.destroyPipeline(pipeline2d);
    resources.device.destroyPipeline(pipeline3d);
    resources.device.destroyPipelineLayout(pipelineLayout);
}

void ParticleCirclePipeline::updateDescriptorSets(const SimulationState &simulationState) {
    auto &descriptorSet = descriptorPool.sets[0];
    Cmn::bindBuffers(resources.device, simulationState.particleCoordinateBuffer.buf, descriptorSet, 0);
    Cmn::bindCombinedImageSampler(resources.device, renderer->colormap.view, renderer->colormap.sampler, descriptorSet, 1);
    Cmn::bindBuffers(resources.device, renderer->uniformBuffer.buf, descriptorSet, 2, vk::DescriptorType::eUniformBuffer);
}

void ParticleCirclePipeline::draw(vk::CommandBuffer &cb, const SimulationState &simulationState) {
    updateDescriptorSets(simulationState);
    vk::Pipeline *pipeline;

    switch (simulationState.parameters.type) {
        case SceneType::SPH_BOX_2D:
            pipeline = &pipeline2d;
            break;
        case SceneType::SPH_BOX_3D:
            pipeline = &pipeline3d;
            break;
        default:
            throw std::runtime_error("ParticleCirclePipeline::draw is not implemented for this scene type");
            break;
    }

    pushStruct.width = resources.extent.width;
    pushStruct.height = resources.extent.height;
    pushStruct.mvp = simulationState.camera->viewProjectionMatrix();

    uint64_t offsets[] = {0UL};
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    cb.bindVertexBuffers(0, 1, &simulationState.particleCoordinateBuffer.buf, offsets);

    cb.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, sizeof(PushStruct), &pushStruct);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorPool.sets[0],
                          0, nullptr);
    cb.draw(simulationState.parameters.numParticles, 1, 0, 0);
}

Background2DPipeline::Background2DPipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, ParticleRenderer *_renderer) : renderer(_renderer) {
    descriptorPool.addStorage(0, 1, vk::ShaderStageFlagBits::eFragment);
    descriptorPool.addSampler(1, 1, vk::ShaderStageFlagBits::eFragment);
    descriptorPool.addUniform(2, 1, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eGeometry);
    descriptorPool.addStorage(3, 1, vk::ShaderStageFlagBits::eFragment);// spatial-lookup
    descriptorPool.addStorage(4, 1, vk::ShaderStageFlagBits::eFragment);// spatial-indices
    descriptorPool.allocate();

    pipelineLayout = createPipelineLayout<PushStruct>(descriptorPool);
    std::vector<GraphicsPipelineBuilder> builders;
    builders.emplace_back(GraphicsPipelineBuilder {
            {{vk::ShaderStageFlagBits::eVertex, "background2d.vert"},
             {vk::ShaderStageFlagBits::eFragment, "background2d.frag"}},
            pipelineLayout,
            renderPass,
            subpass});

    auto pipelines = GraphicsPipelineBuilder::createPipelines(builders);
    pipeline = pipelines[0];

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
}

Background2DPipeline::~Background2DPipeline() {
    resources.device.destroyPipeline(pipeline);
    resources.device.destroyPipelineLayout(pipelineLayout);
}

void Background2DPipeline::draw(vk::CommandBuffer &cb, const SimulationState &simulationState) {
    updateDescriptorSets(simulationState);
    uint64_t offsets[] = {0UL};
    pushStruct.width = resources.extent.width;
    pushStruct.height = resources.extent.height;
    pushStruct.mvp = simulationState.camera->viewProjectionMatrix();

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    cb.bindVertexBuffers(0, 1, &quadVertexBuffer.buf, offsets);
    cb.bindIndexBuffer(quadIndexBuffer.buf, 0UL, vk::IndexType::eUint16);
    cb.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, sizeof(PushStruct), &pushStruct);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorPool.sets[0],
                          0, nullptr);
    cb.drawIndexed(6, 1, 0, 0, 0);// draw quad
}

void Background2DPipeline::updateDescriptorSets(const SimulationState &simulationState) {
    auto &descriptorSet = descriptorPool.sets[0];
    Cmn::bindBuffers(resources.device, simulationState.particleCoordinateBuffer.buf, descriptorSet, 0);
    Cmn::bindCombinedImageSampler(resources.device, renderer->colormap.view, renderer->colormap.sampler, descriptorSet, 1);
    Cmn::bindBuffers(resources.device, renderer->uniformBuffer.buf, descriptorSet, 2, vk::DescriptorType::eUniformBuffer);
    Cmn::bindBuffers(resources.device, simulationState.spatialLookup.buf, descriptorSet, 3);
    Cmn::bindBuffers(resources.device, simulationState.spatialIndices.buf, descriptorSet, 4);
}
