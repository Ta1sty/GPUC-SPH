#include "renderer.h"
#include "helper.h"
#include "initialization.h"

Renderer2D::Renderer2D() {
    vk::ShaderModule particleGeometrySM, particleVertexSM, particleFragmentSM;

    Cmn::createShader(resources.device, particleGeometrySM, shaderPath("particle2d.geom"));
    Cmn::createShader(resources.device, particleVertexSM, shaderPath("particle2d.vert"));
    Cmn::createShader(resources.device, particleFragmentSM, shaderPath("particle2d.frag"));

    vk::PipelineShaderStageCreateInfo shaderStageCI[] {
            {{}, vk::ShaderStageFlagBits::eGeometry, particleGeometrySM},
            {{}, vk::ShaderStageFlagBits::eVertex, particleVertexSM},
            {{}, vk::ShaderStageFlagBits::eFragment, particleFragmentSM},
    };

    vk::GraphicsPipelineCreateInfo particlePipelineCI {
            {},
            3,
            shaderStageCI,
    };

    auto pipelines = resources.device.createGraphicsPipelines(VK_NULL_HANDLE, {particlePipelineCI});
    if (pipelines.result != vk::Result::eSuccess)
        throw std::runtime_error("Pipeline creation failed");

    particlePipeline = pipelines.value[0];
}

Renderer2D::~Renderer2D() {
}

vk::CommandBuffer Renderer2D::run(const SimulationState &state) {
    return vk::CommandBuffer();
}

Renderer::Renderer() : renderer2D() {
}

Renderer::~Renderer() {
}

vk::CommandBuffer Renderer::run(const SimulationState &state) {
    return vk::CommandBuffer();
}
