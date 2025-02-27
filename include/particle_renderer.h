#pragma once

#include "colormaps.h"
#include "simulation_state.h"

class ParticleCirclePipeline;
class Background2DPipeline;
class RayMarcherPipeline;
class BackgroundEnvironmentPipeline;
class ChessboardPipeline;

struct Texture {
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Sampler sampler;

    Texture() = default;
    Texture(const Texture &obj) = delete;
    Texture(Texture &&obj) noexcept {
        *this = std::move(obj);
    }
    Texture &operator=(Texture &&obj) noexcept {
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
    static Texture createFromImage(const std::string &file);
};

class ParticleRenderer {
public:
    ParticleRenderer();
    ParticleRenderer(const ParticleRenderer &particleRenderer) = delete;
    ~ParticleRenderer();
    vk::CommandBuffer run(const SimulationState &simulationState, const RenderParameters &renderParameters);
    void updateCmd(const SimulationState &simulationState, const RenderParameters &renderParameters);
    [[nodiscard]] vk::Image getImage();

    struct SharedResources {
        Texture colormap;
        Texture environmentTexture;
        Buffer uniformBuffer;

        Buffer quadVertexBuffer;
        Buffer quadIndexBuffer;
        Buffer cubeVertexBuffer;

        vk::Sampler depthImageSampler;
        vk::ImageView depthImageView;// written by ParticleRenderer()

        SharedResources();
        ~SharedResources();
    };

private:
    vk::Extent3D imageSize;
    vk::Image colorAttachment;
    vk::ImageView colorAttachmentView;
    vk::DeviceMemory colorAttachmentMemory;
    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;

    vk::RenderPass renderPass;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer commandBuffer;

    std::unique_ptr<BackgroundEnvironmentPipeline> backgroundEnvironmentPipeline;
    std::unique_ptr<ParticleCirclePipeline> particleCirclePipeline;
    std::unique_ptr<Background2DPipeline> background2DPipeline;
    std::unique_ptr<RayMarcherPipeline> rayMarcherPipeline;
    std::unique_ptr<ChessboardPipeline> chessboardPipeline;
    std::shared_ptr<SharedResources> sharedResources;

    struct UniformBufferStruct {
        uint32_t numParticles = 128;
        uint32_t backgroundField = 0;
        uint32_t particleColor = 0;
        float particleRadius = 12.0f;
        float spatialRadius = 0.1f;

    public:
        UniformBufferStruct() = default;
        UniformBufferStruct(const UniformBufferStruct &obj) = default;
        bool operator==(const UniformBufferStruct &obj) const {
            return numParticles == obj.numParticles && backgroundField == obj.backgroundField && particleColor == obj.particleColor && particleRadius == obj.particleRadius && spatialRadius == obj.spatialRadius;
        }
    } uniformBufferContent;
};

/**
 * Compute pass that runs before the actual render pipeline.
 */
class RendererCompute {
public:
    explicit RendererCompute(const RenderParameters &renderParameters);
    RendererCompute(const RendererCompute &obj) = delete;
    ~RendererCompute();
    vk::CommandBuffer run(const SimulationState &simulationState, const RenderParameters &renderParameters);
    void updateCmd(const SimulationState &simulationState, const RenderParameters &renderParameters);

private:
    struct PushStruct {
        uint32_t numParticles;
        float spatialRadius;
    } pushStruct;
    Cmn::DescriptorPool densityGridDescriptorPool;

    vk::PipelineLayout densityGridPipelineLayout;
    vk::Pipeline densityGridPipeline;
    vk::CommandBuffer commandBuffer;
    glm::uvec3 workgroupSize;
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
                {},
                1U,
                &descriptorPool.layout,
                1U,
                &pcr};

        return resources.device.createPipelineLayout(pipelineLayoutCI);
    }


    using SharedResources = std::shared_ptr<ParticleRenderer::SharedResources>;
};

class ParticleCirclePipeline : public GraphicsPipeline {
public:
    ParticleCirclePipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, SharedResources sharedResources);
    ~ParticleCirclePipeline() override;
    void updateDescriptorSets(const SimulationState &simulationState) override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) override;

private:
    struct PushStruct {
        glm::mat4 mvp;
        uint32_t width = 0;
        uint32_t height = 0;
        float targetDensity = 0.0f;
    } pushStruct;

    SharedResources sharedResources;
    Cmn::DescriptorPool descriptorPool;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline2d;
    vk::Pipeline pipeline3d;
};

class Background2DPipeline : public GraphicsPipeline {
public:
    Background2DPipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, SharedResources renderer);
    ~Background2DPipeline() override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) override;
    void updateDescriptorSets(const SimulationState &simulationState) override;

private:
    struct PushStruct {
        glm::mat4 mvp;
        uint32_t width = 0;
        uint32_t height = 0;
    } pushStruct;

    SharedResources sharedResources;
    Cmn::DescriptorPool descriptorPool;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;
};

class RayMarcherPipeline : public GraphicsPipeline {
public:
    RayMarcherPipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, SharedResources renderer);
    ~RayMarcherPipeline() override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState, bool waterShader);
    void updateDescriptorSets(const SimulationState &simulationState) override;
    void copyDensityGridToTexture(vk::CommandBuffer &cb, const SimulationState &simulationState);

private:
    struct PushStruct {
        glm::mat4 mvp;
        glm::vec4 cameraPos;// pad to vec4 for explicit alignment
        glm::vec2 nearFar;
        float targetDensity = 0.0f;
    } pushStruct;

    SharedResources sharedResources;
    Cmn::DescriptorPool descriptorPool;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;
    vk::Pipeline waterPipeline;

    Texture densityGridTexture;
};

class BackgroundEnvironmentPipeline : public GraphicsPipeline {
public:
    BackgroundEnvironmentPipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, SharedResources renderer);
    ~BackgroundEnvironmentPipeline() override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) override;
    void updateDescriptorSets(const SimulationState &simulationState) override;

private:
    struct PushStruct {
        glm::mat4 mvpInv;
        glm::vec4 cameraPos;
    } pushStruct;

    SharedResources sharedResources;
    Cmn::DescriptorPool descriptorPool;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;
};

class ChessboardPipeline : public GraphicsPipeline {
public:
    ChessboardPipeline(const vk::RenderPass &renderPass, uint32_t subpass, const vk::Framebuffer &framebuffer, SharedResources renderer);
    ~ChessboardPipeline() override;
    void draw(vk::CommandBuffer &cb, const SimulationState &simulationState) override;
    void updateDescriptorSets(const SimulationState &simulationState) override;

private:
    struct PushStruct {
        glm::mat4 mvp;
        uint32_t width = 0;
        uint32_t height = 0;
    } pushStruct;

    Cmn::DescriptorPool descriptorPool;
    SharedResources sharedResources;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;
};
