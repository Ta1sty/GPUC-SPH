//
// Created by Benedikt Krimmel on 12/20/2024.
//

#include "helper.h"
#include "task_common.h"
#include "sph_render.h"

namespace sph {

Render2D::Render2D() {
}

void Render2D::init(AppResources &app, const SimulationState &state) {
    vk::ShaderModule particleGeometrySM, particleVertexSM, particleFragmentSM;

    Cmn::createShader(app.device, particleGeometrySM, shaderPath("particle2d.geom"));
    Cmn::createShader(app.device, particleVertexSM, shaderPath("particle2d.vert"));
    Cmn::createShader(app.device, particleFragmentSM, shaderPath("particle2d.frag"));

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

    auto pipelines = app.device.createGraphicsPipelines(VK_NULL_HANDLE, { particlePipelineCI});
    if (pipelines.result != vk::Result::eSuccess)
        throw std::runtime_error("Pipeline creation failed");

    particlePipeline = pipelines.value[0];
}

void Render2D::cleanup(AppResources &app) {

}

void Render2D::render(const SimulationState &state) {

}

Render::Render() : render2D() {

}

void Render::init(AppResources &app, const SimulationState &state) {
    render2D.init(app, state);
}

void Render::cleanup(AppResources &app) {
    render2D.cleanup(app);
}

void Render::render(const SimulationState &state) {
    switch (state.getParameters().type) {
        case SceneType::SPH_BOX_2D:
            render2D.render(state);
            break;
    }
}

} // namespace sph
