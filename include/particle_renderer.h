#pragma once

#include "colormaps.h"
#include "simulation_state.h"

class ParticleCirclePipeline;
class Background2DPipeline;

struct Texture {
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Sampler sampler;

    Texture() = default;
    Texture(const Texture &obj) = delete;
    Texture(Texture &&obj) {
        *this = std::move(obj);
    }
    Texture &operator=(Texture &&obj) {
        image = obj.image;
        memory = obj.memory;
        view = obj.view;
        sampler = obj.sampler;

        obj.image = nullptr;
        obj.memory = nullptr;
        obj.view = nullptr;
        obj.sampler = nullptr;
        return *this;
    }
    ~Texture();

    static Texture createColormapTexture(const std::vector<colormaps::RGB_F32> &colormap);
};

class ParticleRenderer {
public:
    ParticleRenderer();
    ParticleRenderer(const ParticleRenderer &particleRenderer) = delete;
    ~ParticleRenderer();
    vk::CommandBuffer run(const SimulationState &simulationState, const RenderParameters &renderParameters);
    void updateCmd(const SimulationState &simulationState);
    [[nodiscard]] vk::Image getImage();

private:
    vk::Extent3D imageSize;
    vk::Image colorAttachment;
    vk::ImageView colorAttachmentView;
    vk::DeviceMemory colorAttachmentMemory;
    vk::RenderPass renderPass;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer commandBuffer;
    Texture colormap;

    std::unique_ptr<ParticleCirclePipeline> particleCirclePipeline;
    std::unique_ptr<Background2DPipeline> background2DPipeline;

    struct UniformBufferStruct {
        uint32_t numParticles = 128;
        uint32_t backgroundField = 0;
        float particleRadius = 12.0f;
        float spatialRadius = 0.1f;

    public:
        UniformBufferStruct() = default;
        UniformBufferStruct(const UniformBufferStruct &obj) = default;
        bool operator==(const UniformBufferStruct &obj) const {
            return numParticles == obj.numParticles && backgroundField == obj.backgroundField && particleRadius == obj.particleRadius && spatialRadius == obj.spatialRadius;
        }
    } uniformBufferContent;
    Buffer uniformBuffer;

    friend class ParticleCirclePipeline;
    friend class Background2DPipeline;
};

class GraphicsPipeline {
public:
    virtual ~GraphicsPipeline() {}
    virtual void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) {}
    virtual void updateDescriptorSets(const SimulationState &simulationState) {}

public:
    template<typename T>
    static vk::PipelineLayout createPipelineLayout(const Cmn::DescriptorPool &descriptorPool) {
        vk::PushConstantRange pcr {
                vk::ShaderStageFlagBits::eAll, 0, sizeof(T)};

        vk::PipelineLayoutCreateInfo pipelineLayoutCI {
                vk::PipelineLayoutCreateFlags(),
                1U,
                &descriptorPool.layout,
                1U,
                &pcr};

        return resources.device.createPipelineLayout(pipelineLayoutCI);
    }
};

class ParticleCirclePipeline : public GraphicsPipeline {
public:
    ParticleCirclePipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, ParticleRenderer *renderer);
    ~ParticleCirclePipeline() override;
    void updateDescriptorSets(const SimulationState &simulationState) override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) override;

private:
    struct PushStruct {
        glm::mat4 mvp;
        uint32_t width = 0;
        uint32_t height = 0;
    } pushStruct;

    ParticleRenderer *renderer;// TODO replace with pointer to "common render resources"
    Cmn::DescriptorPool descriptorPool;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline2d;
    vk::Pipeline pipeline3d;
};

class Background2DPipeline : public GraphicsPipeline {
public:
    Background2DPipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, ParticleRenderer *renderer);
    ~Background2DPipeline() override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) override;
    void updateDescriptorSets(const SimulationState &simulationState) override;

private:
    struct PushStruct {
        glm::mat4 mvp;
        uint32_t width = 0;
        uint32_t height = 0;
    } pushStruct;

    ParticleRenderer *renderer;// TODO replace with pointer to "common render resources"
    Cmn::DescriptorPool descriptorPool;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;

    Buffer quadVertexBuffer;
    Buffer quadIndexBuffer;
};
