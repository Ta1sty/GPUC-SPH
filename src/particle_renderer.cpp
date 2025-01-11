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

<<<<<<< HEAD
    vk::SubpassDescription particleSubpass {
            {},
            vk::PipelineBindPoint::eGraphics,
            {},
            colorAttachmentReference,
            {},
            nullptr,
            {}};

    vk::SubpassDependency externalDependency {
            VK_SUBPASS_EXTERNAL,
            0,
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::AccessFlagBits::eColorAttachmentWrite},
            {vk::AccessFlagBits::eColorAttachmentWrite}};
=======
    vk::SubpassDescription subpasses[] {
            { // background subpass
                    {},
                    vk::PipelineBindPoint::eGraphics,
                    {},
                    colorAttachmentReference,
                    {},
                    nullptr,
                    {}
            },
            { // particle subpass
                    {},
                    vk::PipelineBindPoint::eGraphics,
                    {},
                    colorAttachmentReference,
                    {},
                    nullptr,
                    {}
            }
    };

    vk::SubpassDependency dependencies[] {
            { // external dependency
                    VK_SUBPASS_EXTERNAL,
                    0,
                    { vk::PipelineStageFlagBits::eColorAttachmentOutput },
                    { vk::PipelineStageFlagBits::eColorAttachmentOutput },
                    { vk::AccessFlagBits::eColorAttachmentWrite },
                    { vk::AccessFlagBits::eColorAttachmentWrite }
            },
            {
                    0,
                    1,
                    { vk::PipelineStageFlagBits::eColorAttachmentOutput },
                    { vk::PipelineStageFlagBits::eColorAttachmentOutput },
                    { vk::AccessFlagBits::eColorAttachmentWrite },
                    { vk::AccessFlagBits::eColorAttachmentWrite }
            }
    };
>>>>>>> main

    vk::RenderPassCreateInfo renderPassCI {
            {},
            1U,
            &colorAttachmentDescription,
<<<<<<< HEAD
            1U,
            &particleSubpass,
            1U,
            &externalDependency};
=======
            2U,
            subpasses,
            2U,
            dependencies
    };
>>>>>>> main

    renderPass = resources.device.createRenderPass(renderPassCI);

    framebuffer = resources.device.createFramebuffer({{}, renderPass, 1, &colorAttachmentView, imageInfo.extent.width, imageInfo.extent.height, imageInfo.extent.depth});

    createColormapTexture(colormaps::viridis);
    createPipeline();

<<<<<<< HEAD
    commandBuffer = resources.device.allocateCommandBuffers(
            {resources.graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1U})[0];
}

vk::CommandBuffer ParticleRenderer::run(const SimulationState &simulationState) {
    // image must be in eColorAttachmentOptimal after the command buffer executed!
    updateDescriptorSets(simulationState);

    commandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::ClearValue clearValue;
    clearValue.color.uint32 = {{0, 0, 0, 0}};
    commandBuffer.beginRenderPass(
            {renderPass, framebuffer, {{0, 0}, resources.extent}, 1, &clearValue},
            vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, particlePipeline);
    uint64_t offsets[] = {0UL};
    commandBuffer.bindVertexBuffers(0, 1, &simulationState.particleCoordinateBuffer.buf, offsets);

    pushStruct.width = resources.extent.width;
    pushStruct.height = resources.extent.height;

    // map [0, 1]^2 into viewport
    pushStruct.mvp = {
            2, 0, 0, -1,
            0, 2, 0, -1,
            0, 0, 0, 0,
            0, 0, 0, 1};
=======
    // quad vertex buffer
    const std::vector<glm::vec2> quadVertices {
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    };
>>>>>>> main

    const std::vector<uint16_t> quadIndices {
        0, 1, 2, 2, 3, 0
    };

    createBuffer(resources.pDevice, resources.device, quadVertices.size() * sizeof(glm::vec2),
                 { vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst },
                 { vk::MemoryPropertyFlagBits::eDeviceLocal },
                 "quadVertexBuffer", quadVertexBuffer.buf, quadVertexBuffer.mem);

    createBuffer(resources.pDevice, resources.device, quadIndices.size() * sizeof(uint16_t),
                 { vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst },
                 { vk::MemoryPropertyFlagBits::eDeviceLocal },
                 "quadIndexBuffer", quadIndexBuffer.buf, quadIndexBuffer.mem);

    fillDeviceWithStagingBuffer(quadVertexBuffer, quadVertices);
    fillDeviceWithStagingBuffer(quadIndexBuffer, quadIndices);

    createBuffer(resources.pDevice, resources.device, sizeof(UniformBufferStruct),
                 { vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst },
                 { vk::MemoryPropertyFlagBits::eDeviceLocal },
                 "render2dUniformBuffer", uniformBuffer.buf, uniformBuffer.mem);
    const std::vector<UniformBufferStruct> uniformBufferVector { uniformBufferContent };
    fillDeviceWithStagingBuffer(uniformBuffer, uniformBufferVector);
}

vk::CommandBuffer ParticleRenderer::run(const SimulationState &simulationState, const RenderParameters &renderParameters) {
    UniformBufferStruct ub {
        simulationState.parameters.numParticles,
        static_cast<uint32_t>(renderParameters.backgroundField),
        renderParameters.particleRadius
    };

    if (!(ub == uniformBufferContent)) {
        uniformBufferContent = ub;
        const std::vector<UniformBufferStruct> uniformBufferVector { uniformBufferContent };
        fillDeviceWithStagingBuffer(uniformBuffer, uniformBufferVector);
    }
<<<<<<< HEAD
    pushStruct.mvp = glm::transpose(pushStruct.mvp);// why are the indices wrong??
=======
>>>>>>> main

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

    resources.device.freeDescriptorSets(descriptorPool, { descriptorSet });
    resources.device.destroyDescriptorPool(descriptorPool);
    resources.device.destroyDescriptorSetLayout(descriptorSetLayout);

    quadVertexBuffer.cleanup();
    quadIndexBuffer.cleanup();
    uniformBuffer.cleanup();
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
<<<<<<< HEAD
            nullptr);
=======
            nullptr
    );
    bindings.emplace_back(
            2,
            vk::DescriptorType::eUniformBuffer,
            1U,
            vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eGeometry
    );
>>>>>>> main

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

<<<<<<< HEAD
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
            {0, 2 * 4, vk::VertexInputRate::eVertex}};

    vk::VertexInputAttributeDescription vertexInputAttributeDescriptions[] {
            {0, 0, vk::Format::eR32G32Sfloat, 0}};
    //
    // Vertex input
    vk::PipelineVertexInputStateCreateInfo vertexInputSCI {
            {},
            1,                              // Vertex binding description  count
            vertexInputBindings,            // List of Vertex Binding Descriptions (data spacing/stride information)
            1,                              // Vertex attribute description count
            vertexInputAttributeDescriptions// List of Vertex Attribute Descriptions (data format and where to bind to/from)
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblySCI {
            {},
            vk::PrimitiveTopology::ePointList,
            false,
    };
=======
>>>>>>> main

    /* =============================== Common Resources =============================== */
    // Viewport & Scissor
<<<<<<< HEAD
    vk::Viewport viewport = {
            0.f,                             // x start coordinate
            (float) resources.extent.height, // y start coordinate
            (float) resources.extent.width,  // Width of viewport
            -(float) resources.extent.height,// Height of viewport
            0.f,                             // Min framebuffer depth,
            1.f                              // Max framebuffer depth
    };
    vk::Rect2D scissor = {
            {0, 0},         // Offset to use region from
            resources.extent// Extent to describe region to use, starting at offset
=======
    const vk::Viewport viewport = {
            0.f, // x start coordinate
            (float)resources.extent.height, // y start coordinate
            (float)resources.extent.width, // Width of viewport
            -(float)resources.extent.height, // Height of viewport
            0.f, // Min framebuffer depth,
            1.f // Max framebuffer depth
    };
    const vk::Rect2D scissor = {
            {0, 0}, // Offset to use region from
            resources.extent // Extent to describe region to use, starting at offset
>>>>>>> main
    };

    const vk::PipelineViewportStateCreateInfo viewportSCI = {
            {},
            1,        // Viewport count
            &viewport,// Viewport used
            1,        // Scissor count
            &scissor  // Scissor used
    };

    // Rasterizer
    const vk::PipelineRasterizationStateCreateInfo rasterizationSCI = {
            {},
            false,// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
            false,
            // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
<<<<<<< HEAD
            vk::PolygonMode::eFill,     // How to handle filling points between vertices
            vk::CullModeFlagBits::eBack,// Which face of a tri to cull
            vk::FrontFace::eClockwise,  // Winding to determine which side is front
            false,                      // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
=======
            vk::PolygonMode::eFill, // How to handle filling points between vertices
            vk::CullModeFlagBits::eBack, // Which face of a tri to cull
            vk::FrontFace::eCounterClockwise, // Winding to determine which side is front
            false, // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
>>>>>>> main
            0.f,
            0.f,
            0.f,
            1.f// How thick lines should be when drawn
    };

    const vk::PipelineMultisampleStateCreateInfo multisampleSCI = {
            {},
            vk::SampleCountFlagBits::e1,// Number of samples to use per fragment
            false,                      // Enable multisample shading or not
            0.f,
            nullptr,
            false,
<<<<<<< HEAD
            false};
    // Depth stencil creation
    vk::PipelineDepthStencilStateCreateInfo depthStencilSCI = {
            {},
            true,
            false,
            vk::CompareOp::eLess,
            false,
            false,
            {},
            {},
            0.f,
            0.f};
=======
            false
    };

    // Depth stencil creation
    const vk::PipelineDepthStencilStateCreateInfo depthStencilSCI = {
            {}, vk::False
//            {}, vk::True, vk::False, vk::CompareOp::eAlways, vk::False, vk::False, {}, {}, 0.0f, 0.0f
//            {}, true, false, vk::CompareOp::eLess, false, false, {}, {}, 0.f, 0.f
    };
>>>>>>> main

    /* =============================== Particle Pipeline =============================== */
    vk::ShaderModule particleGeometrySM, particleVertexSM, particleFragmentSM;

    Cmn::createShader(resources.device, particleVertexSM, shaderPath("particle2d.vert"));
    Cmn::createShader(resources.device, particleGeometrySM, shaderPath("particle2d.geom"));
    Cmn::createShader(resources.device, particleFragmentSM, shaderPath("particle2d.frag"));

    // essentially copy pasted from project.h
    // TODO refactor and reuse for future pipeline initializations
    vk::PipelineShaderStageCreateInfo particleShaderStageCI[] {
            {{}, vk::ShaderStageFlagBits::eVertex, particleVertexSM, "main", nullptr},
            {{}, vk::ShaderStageFlagBits::eGeometry, particleGeometrySM, "main", nullptr},
            {{}, vk::ShaderStageFlagBits::eFragment, particleFragmentSM, "main", nullptr},
    };

    vk::VertexInputBindingDescription particleVertexInputBindings[] {
            { 0, 2 * 4, vk::VertexInputRate::eVertex }
    };

    vk::VertexInputAttributeDescription particleVertexInputAttributeDescriptions[] {
            { 0, 0, vk::Format::eR32G32Sfloat, 0 }
    };
//
    // Vertex input
    vk::PipelineVertexInputStateCreateInfo particleVertexInputSCI {
            {},
            1, // Vertex binding description  count
            particleVertexInputBindings, // List of Vertex Binding Descriptions (data spacing/stride information)
            1, // Vertex attribute description count
            particleVertexInputAttributeDescriptions // List of Vertex Attribute Descriptions (data format and where to bind to/from)
    };

    vk::PipelineInputAssemblyStateCreateInfo particleInputAssemblySCI {
            {},
            vk::PrimitiveTopology::ePointList,
            false,
    };


    vk::PipelineColorBlendAttachmentState particleColorBlendAttachmentState {
            true,
            vk::BlendFactor::eSrcAlpha,
            vk::BlendFactor::eOneMinusSrcAlpha,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo particleColorBlendSCI {
            {},
            false,
            {},
            1,
<<<<<<< HEAD
            &colorBlendAttachmentState,
            {}};
=======
            &particleColorBlendAttachmentState,
            {}
    };
>>>>>>> main

    vk::GraphicsPipelineCreateInfo particlePipelineCI {
            {},
            3,
            particleShaderStageCI,
            &particleVertexInputSCI,
            &particleInputAssemblySCI,
            nullptr,
            &viewportSCI,
            &rasterizationSCI,
            &multisampleSCI,
            &depthStencilSCI,
            &particleColorBlendSCI,
            nullptr,
            particlePipelineLayout,
            renderPass,
            1, // subpass
            {},
            0
    };

    /* =============================== Background Pipeline =============================== */
    vk::ShaderModule backgroundVertexSM, backgroundFragmentSM;

    Cmn::createShader(resources.device, backgroundVertexSM, shaderPath("background2d.vert"));
    Cmn::createShader(resources.device, backgroundFragmentSM, shaderPath("background2d.frag"));

    // essentially copy pasted from project.h
    // TODO refactor and reuse for future pipeline initializations
    vk::PipelineShaderStageCreateInfo backgroundShaderStageCI[] {
            {{}, vk::ShaderStageFlagBits::eVertex, backgroundVertexSM, "main", nullptr},
            {{}, vk::ShaderStageFlagBits::eFragment, backgroundFragmentSM, "main", nullptr},
    };


    vk::VertexInputBindingDescription backgroundVertexInputBindings[] {
            { 0, 2 * 4, vk::VertexInputRate::eVertex }
    };

    vk::VertexInputAttributeDescription backgroundVertexInputAttributeDescriptions[] {
            { 0, 0, vk::Format::eR32G32Sfloat, 0 }
    };

    vk::PipelineVertexInputStateCreateInfo backgroundVertexInputSCI {
            {},
            1, // Vertex binding description  count
            backgroundVertexInputBindings, // List of Vertex Binding Descriptions (data spacing/stride information)
            1, // Vertex attribute description count
            backgroundVertexInputAttributeDescriptions // List of Vertex Attribute Descriptions (data format and where to bind to/from)
    };

    vk::PipelineInputAssemblyStateCreateInfo backgroundInputAssemblySCI {
            {},
            vk::PrimitiveTopology::eTriangleList,
            false,
    };


    vk::GraphicsPipelineCreateInfo backgroundPipelineCI {
            {},
            2,
            backgroundShaderStageCI,
            &backgroundVertexInputSCI,
            &backgroundInputAssemblySCI,
            nullptr,
            &viewportSCI,
            &rasterizationSCI,
            &multisampleSCI,
            &depthStencilSCI,
            &particleColorBlendSCI, // TODO
            nullptr,
            particlePipelineLayout,
            renderPass,
            0,// subpass
            {},
            0};

<<<<<<< HEAD
    auto pipelines = resources.device.createGraphicsPipelines(VK_NULL_HANDLE, {particlePipelineCI});
=======
    auto pipelines = resources.device.createGraphicsPipelines(VK_NULL_HANDLE, { backgroundPipelineCI, particlePipelineCI });
>>>>>>> main
    if (pipelines.result != vk::Result::eSuccess)
        throw std::runtime_error("Pipeline creation failed");

    backgroundPipeline = pipelines.value[0];
    particlePipeline = pipelines.value[1];

    resources.device.destroyShaderModule(particleVertexSM);
    resources.device.destroyShaderModule(particleGeometrySM);
    resources.device.destroyShaderModule(particleFragmentSM);

    resources.device.destroyShaderModule(backgroundVertexSM);
    resources.device.destroyShaderModule(backgroundFragmentSM);
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
}

void ParticleRenderer::updateCmd(const SimulationState &simulationState) {
    if (commandBuffer == nullptr)
        commandBuffer = resources.device.allocateCommandBuffers(
                { resources.graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1U }
        )[0];
    else
        commandBuffer.reset();

    // image must be in eColorAttachmentOptimal after the command buffer executed!
    updateDescriptorSets(simulationState);
    pushStruct.width = resources.extent.width;
    pushStruct.height = resources.extent.height;

    // map [0, 1]^2 into viewport
    pushStruct.mvp = simulationState.camera->viewProjectionMatrix();

    commandBuffer.begin(vk::CommandBufferBeginInfo {});
    uint64_t offsets[] = { 0UL };

    vk::ClearValue clearValue;
    clearValue.color.uint32 = {{ 0, 0, 0, 0 }};
    commandBuffer.beginRenderPass(
            { renderPass, framebuffer, {{ 0, 0}, resources.extent }, 1, &clearValue},
            vk::SubpassContents::eInline);
    /* ========== Background Subpass ========== */
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, backgroundPipeline);
    commandBuffer.bindVertexBuffers(0, 1, &quadVertexBuffer.buf, offsets);
    commandBuffer.bindIndexBuffer(quadIndexBuffer.buf, 0UL, vk::IndexType::eUint16);
    commandBuffer.pushConstants(particlePipelineLayout, vk::ShaderStageFlagBits::eAll, 0, sizeof(PushStruct), &pushStruct);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, particlePipelineLayout, 0, 1, &descriptorSet,
                                     0, nullptr);
    commandBuffer.drawIndexed(6, 1, 0, 0, 0); // draw quad

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
