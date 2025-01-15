#pragma once

#include "colormaps.h"
#include "simulation_state.h"

class ParticleRenderer;

class ParticleRenderer2D {
public:
    explicit ParticleRenderer2D(ParticleRenderer *renderer);
    ParticleRenderer2D(const ParticleRenderer2D &particleRenderer) = delete;
    ~ParticleRenderer2D();
    vk::CommandBuffer run(const SimulationState &simulationState, const RenderParameters &renderParameters);
    void updateCmd(const SimulationState &simulationState);

private:
    void createPipelines();
    void createColormapTexture(const std::vector<colormaps::RGB_F32> &colormap);
    void updateDescriptorSets(const SimulationState &simulationState);

    struct PushStruct {
        glm::mat4 mvp;
        uint32_t width = 0;
        uint32_t height = 0;
    } pushStruct;

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

    ParticleRenderer *renderer;

    vk::Framebuffer framebuffer;

    vk::Pipeline particlePipeline;
    vk::Pipeline backgroundPipeline;
    vk::PipelineLayout particlePipelineLayout;
    vk::RenderPass renderPass;

    vk::CommandBuffer commandBuffer;

    vk::Image colormapImage;
    vk::DeviceMemory colormapImageMemory;
    vk::ImageView colormapImageView;
    vk::Sampler colormapSampler;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;

    Buffer quadVertexBuffer;
    Buffer quadIndexBuffer;

    Buffer uniformBuffer;
};

class ParticleRenderer {
public:
    ParticleRenderer();
    ParticleRenderer(const ParticleRenderer2D &particleRenderer) = delete;
    ~ParticleRenderer();
    vk::CommandBuffer run(const SimulationState &simulationState, const RenderParameters &renderParameters);
    void updateCmd(const SimulationState &simulationState);
    [[nodiscard]] vk::Image getImage();

private:
    vk::Extent3D imageSize;
    std::unique_ptr<ParticleRenderer2D> renderer2D;
    vk::Image colorAttachment;
    vk::ImageView colorAttachmentView;
    vk::DeviceMemory colorAttachmentMemory;

    friend class ParticleRenderer2D;
};