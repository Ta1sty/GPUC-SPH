#pragma once

#include <iostream>
#include <cstdlib>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "tiny_obj_loader.h"
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <vector>
#include <memory>
#include "initialization.h"
#include "utils.h"
#include "task_common.h"
#include "render.h"
#include "helper.h"
#include "simulation_parameters.h"

struct ProjectData {
    /**
     * Making unused code work with the new framework .-.
     */
    SimulationParameters simulationParameters;
    RenderParameters renderParameters;
    UiBindings uiBindings;
    inline ProjectData() : uiBindings({}, simulationParameters, renderParameters, nullptr) {}

    struct PushConstant {
        glm::mat4 mvp;
        glm::vec4 pos;
        float dT; // timestep
    };

    PushConstant push;
    uint32_t particleCount;
    uint32_t triangleCount;
    vk::Sampler textureSampler;
    vk::Image textureImage;
    vk::ImageView textureView;
    vk::DeviceMemory textureImageMemory;
    const int V_RESx = 128;
    const int V_RESy = 128;
    const int V_RESz = 128;

    Buffer gAlive, gPosLife, gVelMass, gTriangleSoup, gNormals, gForceLines;
    uint32_t NUM_FORCE_LINES;
    // Descriptor & Pipeline Layout
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::DescriptorSetLayout descriptorSetLayout;

    vk::PipelineLayout layout;

    // Descriptor Pool
    vk::DescriptorPool descriptorPool;

    // Per-dispatch data
    vk::DescriptorSet descriptorSet;
};

class ProjectSolution {
public:
    ProjectSolution(AppResources& app, ProjectData& datad, uint32_t workGroupSize_x, uint32_t triangleCacheSize);

    void cleanup();

    void compute();
    std::vector<float> result() const;

    float mstime, insideTime;

private:
    void make3DTexture();
    void makeSampler();
    void initParticles();

    AppResources& app;
    ProjectData& data;
    uint32_t workGroupSize_x;
    uint32_t triangleCacheSize;
    // Local PPS Pipeline
    vk::ShaderModule particleShader, scanShader, reorgShader;
    vk::Pipeline particlePipeline, scanPipeline, reorgPipeline;

    std::vector<vk::DescriptorSetLayoutBinding> scanBindings, reorgBindings;
    vk::DescriptorSetLayout scanDescriptorSetLayout, reorgDescriptorSetLayout;
    vk::PipelineLayout scanLayout, reorgLayout;

    vk::CommandBuffer cb;
};

class ProjectRender {
public:
    AppResources& app;
    Render& render;
    std::unique_ptr<ImguiUi> imguiUi = nullptr;

    vk::Pipeline opaquePipeline;
    vk::Pipeline transparentPipeline;
    vk::Pipeline linesPipeline;

    bool renderForceLines = false;
    bool fPress = false;

    ProjectRender(AppResources& app, Render& render): app(app), render(render) {
        imguiUi = std::make_unique<ImguiUi>();
    };

    void createPipeline(vk::Pipeline& pipeline, ProjectData& data, const std::string& vertex,
                        const std::string& fragment,
                        vk::PrimitiveTopology primitiveType, vk::PolygonMode polygonMode,
                        vk::PipelineColorBlendAttachmentState blendState, vk::CullModeFlagBits cullMode,
                        unsigned int subpass, bool depthWrite) {
        vk::ShaderModule vertexM, fragmentM;
        // Read in SPIR-V code of shaders
        Cmn::createShader(app.device, vertexM, vertex);
        Cmn::createShader(app.device, fragmentM, fragment);

        // Put shader stage creation info in to array
        // Graphics Pipeline creation info requires array of shader stage creates
        vk::PipelineShaderStageCreateInfo shaderStageCI[] = {
            {{}, vk::ShaderStageFlagBits::eVertex, vertexM, "main", nullptr},
            {{}, vk::ShaderStageFlagBits::eFragment, fragmentM, "main", nullptr},
        };

        // Vertex input
        vk::PipelineVertexInputStateCreateInfo vertexInputSCI = {
            {},
            0, // Vertex binding description  count
            nullptr, // List of Vertex Binding Descriptions (data spacing/stride information)
            0, // Vertex attribute description count
            nullptr // List of Vertex Attribute Descriptions (data format and where to bind to/from)
        };
        // Input assembly
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblySCI = {
            {},
            primitiveType, // Primitive type to assemble vertices as
            false // Allow overriding of "strip" topology to start new primitives
        };
        // Viewport & Scissor
        vk::Viewport viewport = {
            0.f, // x start coordinate
            (float)app.extent.height, // y start coordinate
            (float)app.extent.width, // Width of viewport
            -(float)app.extent.height, // Height of viewport
            0.f, // Min framebuffer depth,
            1.f // Max framebuffer depth
        };
        vk::Rect2D scissor = {
            {0, 0}, // Offset to use region from
            app.extent // Extent to describe region to use, starting at offset
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
            polygonMode, // How to handle filling points between vertices
            cullMode, // Which face of a tri to cull
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
            {}, true, depthWrite, vk::CompareOp::eLess, false, false, {}, {}, 0.f, 0.f
        };
        // -- BLENDING --
        // Blending decides how to blend a new colour being written to a fragment, with the old value
        // Blend Attachment State (how blending is handled)
        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = blendState;
        // Colours to apply blending to
        colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        vk::PipelineColorBlendStateCreateInfo colorBlendSCI = {
            {},
            false, //logicOpEnable - Alternative to calculations is to use logical operations
            {},
            1,
            &colorBlendAttachmentState, //pAttachments
            {}
        };
        // Graphics Pipeline Creation
        vk::GraphicsPipelineCreateInfo cI = {
            {},
            2, // Number of shader stages
            &shaderStageCI[0], // List of shader stages
            &vertexInputSCI, // All the fixed function pipeline states
            &inputAssemblySCI,
            nullptr,
            &viewportSCI,
            &rasterizationSCI,
            &multisampleSCI,
            &depthStencilSCI,
            &colorBlendSCI,
            nullptr,
            data.layout, // Pipeline Layout pipeline should use
            render.renderPass, // Render pass description the pipeline is compatible with
            subpass, // Subpass of render pass to use with pipeline
            // Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
            {}, //basePipelineHandle - Existing pipeline to derive from...
            0 //basePipelineIndex - or index of pipeline being created to derive from (in case creating multiple at once)
        };

        // Create Graphics Pipeline
        auto pipelines = app.device.createGraphicsPipelines(VK_NULL_HANDLE, {cI});
        if (pipelines.result != vk::Result::eSuccess)
            throw std::runtime_error("Pipeline creation failed");
        pipeline = pipelines.value[0];

        // Destroy Shader Modules, no longer needed after Pipeline created
        app.device.destroyShaderModule(vertexM);
        app.device.destroyShaderModule(fragmentM);
    }

    void prepare(ProjectData& data) {
        imguiUi->initCommandBuffers();

        {
            // Opaque
            vk::PipelineColorBlendAttachmentState blendState = {false};

            createPipeline(opaquePipeline, data, workingDir + "build/shaders/soup.vert.spv",
                           workingDir + "build/shaders/phong.frag.spv", vk::PrimitiveTopology::eTriangleList,
                           vk::PolygonMode::eFill, blendState, vk::CullModeFlagBits::eBack, 0, true);
        }
        {
            // Transparent
            vk::PipelineColorBlendAttachmentState blendState = {
                true, vk::BlendFactor::eOne, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eOne, vk::BlendOp::eAdd
            };

            createPipeline(transparentPipeline, data, workingDir + "build/shaders/particle.vert.spv",
                           workingDir + "build/shaders/white.frag.spv", vk::PrimitiveTopology::ePointList,
                           vk::PolygonMode::eFill, blendState, vk::CullModeFlagBits::eNone, 1, false);
        }
        {
            // Lines
            vk::PipelineColorBlendAttachmentState blendState = {
                true, vk::BlendFactor::eOne, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eOne, vk::BlendOp::eAdd
            };

            createPipeline(linesPipeline, data, workingDir + "build/shaders/lines.vert.spv",
                           workingDir + "build/shaders/white.frag.spv", vk::PrimitiveTopology::eTriangleList,
                           vk::PolygonMode::eLine,
                           blendState, vk::CullModeFlagBits::eNone, 1, false);
        }
    }

    void destroy() {
        imguiUi = nullptr;
        app.device.destroyPipeline(opaquePipeline);
        app.device.destroyPipeline(transparentPipeline);
        app.device.destroyPipeline(linesPipeline);
    }

    void renderFrame(ProjectData& data) {
        if (glfwGetKey(app.window, GLFW_KEY_F) == GLFW_PRESS) {
            if (!fPress) {
                renderForceLines = !renderForceLines;
                fPress = true;
            }
        }
        else if (fPress) fPress = false;

        render.renderFrame(
            [&](vk::CommandBuffer& cb) {
                cb.bindPipeline(vk::PipelineBindPoint::eGraphics, opaquePipeline);
                ProjectData::PushConstant pC = {
                    render.camera->viewProjectionMatrix(), glm::vec4(render.camera->position, 0.f)
                };
                cb.pushConstants(data.layout, vk::ShaderStageFlagBits::eAll, 0, sizeof(pC), &pC);
                cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, data.layout, 0, 1, &data.descriptorSet, 0,
                                      nullptr);
                cb.draw(3 * data.triangleCount, 1, 0, 0);
            },
            [&](vk::CommandBuffer& cb) {
                {
                    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, transparentPipeline);
                    ProjectData::PushConstant pC = {
                        render.camera->viewProjectionMatrix(), glm::vec4(render.camera->position, 0.f)
                    };
                    cb.pushConstants(data.layout, vk::ShaderStageFlagBits::eAll, 0, sizeof(pC), &pC);
                    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, data.layout, 0, 1, &data.descriptorSet, 0,
                                          nullptr);
                    cb.draw(data.particleCount, 1, 0, 0);
                }
                if (renderForceLines) {
                    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, linesPipeline);
                    ProjectData::PushConstant pC = {
                        render.camera->viewProjectionMatrix(), glm::vec4(render.camera->position, 0.f)
                    };
                    cb.pushConstants(data.layout, vk::ShaderStageFlagBits::eAll, 0, sizeof(pC), &pC);
                    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, data.layout, 0, 1, &data.descriptorSet, 0,
                                          nullptr);
                    cb.draw(3 * 10 * 10 * 10, 1, 0, 0);
                }
            },
            [&](uint32_t index){
                return imguiUi->updateCommandBuffer(index, data.uiBindings);
            }
        );
    }
};

class Project {
    AppResources& app;
    ProjectRender render;

public:
    ProjectData data;

    Project(AppResources& app, Render& render, uint32_t particleCount = 20,
            std::string objName = workingDir + "Assets/cubeJump.obj");

    void loop(ProjectSolution& solution);

    void cleanup();

private:
    /*  1- read, modify and setup the data for the texture as an std::vector<float>
        2- fill a staging buffer with data in 1
        3- Create an Image, allocate its memory and bind them
        4- Change the format of the image so that we can copy the data to it (eTransferDstOptimal)
        5- copy data from 2 to 3 with new format of 4
        6- Change the format again to make it accessible to the shader (eShaderReadOnlyOptimal)
        7- Destroy the staging buffer and free its memory
        ===================================================
        members modified:
        vk::Image textureImage
        vk::DeviceMemory textureImageMemory
    */
    void make3DTexture() {
        Buffer staging;

        {
            //  SCOPE : Read and Fill a vector
            FILE* fin = fopen((workingDir + "Assets/helix_float.raw").c_str(), "rb");
            if (!fin) {
                std::cout << "Unable to open volumetric data file." << std::endl;
                return;
            }

            // pVolume holds the data for the texture
            size_t tex_size = data.V_RESx * data.V_RESy * data.V_RESz;
            std::vector<std::array<float, 4>> pVolume(tex_size);
            // Flip the y and z axis of the volume and adjust the forces a little
            unsigned int i = 0;
            for (unsigned int z = 0; z < data.V_RESz; z++) {
                for (unsigned int y = 0; y < data.V_RESy; y++) {
                    for (unsigned int x = 0; x < data.V_RESx; x++) {
                        std::array<float, 4> pForce;
                        fread(&pForce, sizeof(float), 3, fin);
                        pForce[2] *= 2.f;
                        float scale = (1.f - (float)z / (float)data.V_RESz) * 1.f;
                        pForce[0] *= scale;
                        pForce[1] *= scale;
                        pForce[2] *= scale * (1.f - exp(-(float)z / (float)data.V_RESz * 5.f));
                        pForce[3] = 0.f;

                        //std::swap(pForce[1], pForce[2]);
                        // 2 -> 0,
                        //pForce={ pForce[2], pForce[0], pForce[1], 0.f};

                        pVolume[data.V_RESx * data.V_RESz * z + data.V_RESy * y + x] = pForce;
                    }
                }
            }
            fclose(fin);
            createBuffer(app.pDevice, app.device, tex_size * sizeof(std::array<float, 4>),
                         vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
                         "staging_tex", staging.buf, staging.mem);

            fillDeviceBuffer(app.device, staging.mem, pVolume);
        }

        auto makeImage = [&](vk::Image& image, vk::ImageView& imageView, vk::DeviceMemory& img_mem) {
            vk::ImageCreateInfo imgInfo(vk::ImageCreateFlags{}, vk::ImageType::e3D, // VkImageCreateFlags, VkImageType
                                        vk::Format::eR32G32B32A32Sfloat, // VkImageFormat
                                        vk::Extent3D(data.V_RESx, data.V_RESy, data.V_RESz), // w,h,depth
                                        1, 1, // mipLevels, arrayLayers,
                                        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                                        // VkSampleCountFlagBits, VkImageTiling
                                        vk::ImageUsageFlagBits::eSampled | // VkImageUsageFlags
                                        vk::ImageUsageFlagBits::eTransferDst,
                                        vk::SharingMode::eExclusive, // VkSharingMode
                                        0, nullptr, // queueFamilyIndexCount, *pQueueFamilyIndices
                                        vk::ImageLayout::eUndefined // VkImageLayout
            );

            vk::MemoryRequirements img_mem_req;
            if (app.device.createImage(&imgInfo, nullptr, &image) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create image!");
            }

            app.device.getImageMemoryRequirements(image, &img_mem_req);
            vk::MemoryAllocateInfo allocInfo(img_mem_req.size,
                                             findMemoryType(img_mem_req.memoryTypeBits,
                                                            vk::MemoryPropertyFlagBits::eDeviceLocal, app.pDevice));

            img_mem = app.device.allocateMemory(allocInfo, nullptr);

            app.device.bindImageMemory(image, img_mem, 0);

            vk::ImageViewCreateInfo viewInfo(
                {}, image, vk::ImageViewType::e3D, vk::Format::eR32G32B32A32Sfloat,
                {{}, {}, {}, {}}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

            imageView = app.device.createImageView(viewInfo);
        };

        makeImage(data.textureImage, data.textureView, data.textureImageMemory);
        setObjectName(app.device, data.textureImage, "3DTexImage");
        setObjectName(app.device, data.textureView, "3DTexView");
        setObjectName(app.device, data.textureImageMemory, "3DTexImageMemory");

        transitionImageLayout(
            app.device, app.transferCommandPool, app.transferQueue,
            data.textureImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(app.device, app.transferCommandPool, app.transferQueue, staging.buf, data.textureImage,
                          data.V_RESx, data.V_RESy, data.V_RESz);
        transitionImageLayout(
            app.device, app.computeCommandPool, app.computeQueue,
            data.textureImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        app.device.destroyBuffer(staging.buf);
        app.device.freeMemory(staging.mem);
    }

    void makeForceLines(std::vector<std::array<float, 4>>& pForce, uint32_t NUM_FORCE_LINES = 50) {
        //scatter force field sampling points
        pForce.resize(NUM_FORCE_LINES * 2);
        for (int i = 0; i < NUM_FORCE_LINES; i++) {
            pForce[2 * i] = {
                float(rand()) / float(RAND_MAX),
                float(rand()) / float(RAND_MAX),
                float(rand()) / float(RAND_MAX),
                0.0f
            };

            pForce[2 * i + 1] = pForce[2 * i];
            pForce[2 * i + 1][3] = 1.f;
        }
    }

    void makeSampler() {
        vk::PhysicalDeviceProperties properties = app.pDevice.getProperties();
        vk::SamplerCreateInfo samplerInfo(
            vk::SamplerCreateFlags{},
            vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            0.f,
            vk::Bool32(false),
            properties.limits.maxSamplerAnisotropy,
            vk::Bool32(false),
            vk::CompareOp::eAlways, 0.f, 0.f,
            vk::BorderColor::eIntOpaqueBlack,
            vk::Bool32(false)
        );
        data.textureSampler = app.device.createSampler(samplerInfo);
        setObjectName(app.device, data.textureSampler, "3DTexSampler");
    }

    /*
        Three buffers filled:
        gVelMass
        gPosLife
        gAlive
    */
    void initParticles() {
        //fill the array with some values
        std::vector<std::array<float, 4>> pPosLife, pVelMass;
        pPosLife.resize(2 * data.particleCount);
        pVelMass.resize(2 * data.particleCount);

        for (unsigned int i = 0; i < data.particleCount; i++) {
            pPosLife[i][0] = (float(rand()) / float(RAND_MAX) * 0.5f + 0.25f);
            pPosLife[i][1] = (float(rand()) / float(RAND_MAX) * 0.5f + 0.25f);
            pPosLife[i][2] = (float(rand()) / float(RAND_MAX) * 0.5f + 0.25f);
            pPosLife[i][3] = 4.5f * (float(rand()) / float(RAND_MAX));

            pVelMass[i][0] = 0.f;
            pVelMass[i][1] = 0.f;
            pVelMass[i][2] = 0.f;
            pVelMass[i][3] = (1.f + float(rand()) / float(RAND_MAX)) * 1.5f;
        }
        std::vector<uint32_t> pAlive(data.particleCount, 1); // init with 1
        pAlive.resize(2 * data.particleCount, 0); // fill rest with 0

        fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, data.gVelMass,
                                    pVelMass);
        fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, data.gPosLife,
                                    pPosLife);
        fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, data.gAlive,
                                    pAlive);
    }

    std::vector<std::array<float, 4>> getTriangles(std::string dataFile, std::vector<std::array<float, 4>>& normals) {
        std::vector<std::array<float, 4>> triangleSoup;
        tinyobj::ObjReaderConfig reader_config;
        // reader_config.mtl_search_path = "./"; // Path to material files

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(dataFile, reader_config)) {
            if (!reader.Error().empty()) {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
            exit(1);
        }

        if (!reader.Warning().empty()) {
            std::cout << "TinyObjReader: " << reader.Warning();
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();
        triangleSoup.reserve(attrib.GetVertices().size());
        normals.reserve(attrib.GetVertices().size());
        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                float vec[4];
                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    // * for us, fv = 3 (triangles)
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    float vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                    float vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                    float vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                    triangleSoup.push_back({vx, vy, vz, 0});

                    // Check if `normal_index` is zero or positive. negative = no normal data
                    if (idx.normal_index >= 0) {
                        tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                        tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                        tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                        normals.push_back({nx, ny, nz, 0.f});
                    }
                }
                index_offset += fv;
            }
        }
        return triangleSoup;
    }
};
