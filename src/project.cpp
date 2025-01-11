#include "project.h"

#include <cstdlib>
#include <iostream>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "host_timer.h"
#include "initialization.h"
#include "stb_image_write.h"
#include "task_common.h"
#include "utils.h"
#include <fstream>
#include <vector>
#include <vulkan/vulkan.hpp>

Project::Project(AppResources &app, Render &render, const uint32_t particleCount,
                 std::string objName) : app(app), render(app, render) {
    data.NUM_FORCE_LINES = 100;
    data.particleCount = particleCount;

    std::vector<std::array<float, 4>> tSoup, normals, pForce;

    make3DTexture();
    makeSampler();
    makeForceLines(pForce, data.NUM_FORCE_LINES);

    tSoup = getTriangles(objName, normals);

    data.triangleCount = tSoup.size() / 3;

    Cmn::addStorage(data.bindings, 0);             // gAlive
    Cmn::addStorage(data.bindings, 1);             // gPosLife
    Cmn::addStorage(data.bindings, 2);             // gVelMass
    Cmn::addStorage(data.bindings, 3);             // gTriangleSoup
    Cmn::addCombinedImageSampler(data.bindings, 4);// texture sampler
    Cmn::addStorage(data.bindings, 5);             // gNormals
    Cmn::addStorage(data.bindings, 6);             // gForceLines

    Cmn::createDescriptorSetLayout(app.device, data.bindings, data.descriptorSetLayout);

    using BFlag = vk::BufferUsageFlagBits;
    auto makeDLocalBuffer = [&](vk::BufferUsageFlags usage, vk::DeviceSize size, std::string name) -> Buffer {
        Buffer b;
        createBuffer(app.pDevice, app.device, size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, name, b.buf,
                     b.mem);
        return b;
    };

    data.gAlive = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                   data.particleCount * 2 * sizeof(uint32_t), "gAlive");
    data.gPosLife = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                     data.particleCount * 2 * sizeof(float) * 4, "gPosLife");
    data.gVelMass = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                     data.particleCount * 2 * sizeof(float) * 4, "gVelMass");
    data.gTriangleSoup = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                          tSoup.size() * sizeof(float) * 4, "gTriangleSoup");
    data.gNormals = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                     normals.size() * sizeof(float) * 4, "gNormals");
    data.gForceLines = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                        pForce.size() * sizeof(float) * 4, "gForceLines");

    initParticles();// fills gAlive, gPosLife and gVelMass
    fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue,
                                data.gTriangleSoup, tSoup);
    fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, data.gNormals,
                                normals);
    fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue,
                                data.gForceLines, pForce);

    Cmn::createDescriptorPool(app.device, data.bindings, data.descriptorPool);
    Cmn::allocateDescriptorSet(app.device, data.descriptorSet, data.descriptorPool, data.descriptorSetLayout);

    Cmn::bindBuffers(app.device, data.gAlive.buf, data.descriptorSet, 0);
    Cmn::bindBuffers(app.device, data.gPosLife.buf, data.descriptorSet, 1);
    Cmn::bindBuffers(app.device, data.gVelMass.buf, data.descriptorSet, 2);
    Cmn::bindBuffers(app.device, data.gTriangleSoup.buf, data.descriptorSet, 3);
    Cmn::bindCombinedImageSampler(app.device, data.textureView, data.textureSampler, data.descriptorSet, 4);
    Cmn::bindBuffers(app.device, data.gNormals.buf, data.descriptorSet, 5);
    Cmn::bindBuffers(app.device, data.gForceLines.buf, data.descriptorSet, 6);

    vk::PushConstantRange pcr(vk::ShaderStageFlagBits::eAll, 0, sizeof(ProjectData::PushConstant));

    vk::PipelineLayoutCreateInfo pipInfo(vk::PipelineLayoutCreateFlags(), 1U, &data.descriptorSetLayout, 1U, &pcr);
    data.layout = app.device.createPipelineLayout(pipInfo);

    data.push.dT = 0.008;
    this->render.prepare(data);
}


void Project::loop(ProjectSolution &solution) {
    solution.compute();
    render.renderFrame(data);
}

void Project::cleanup() {
    app.device.destroyImage(data.textureImage);
    app.device.freeMemory(data.textureImageMemory);
    app.device.destroyImageView(data.textureView);
    app.device.destroySampler(data.textureSampler);

    app.device.destroyDescriptorPool(data.descriptorPool);

    app.device.destroyPipelineLayout(data.layout);
    app.device.destroyDescriptorSetLayout(data.descriptorSetLayout);
    data.bindings.clear();

    auto Bclean = [&](Buffer &b) {
        app.device.destroyBuffer(b.buf);
        app.device.freeMemory(b.mem);
    };

    Bclean(data.gAlive);
    Bclean(data.gVelMass);
    Bclean(data.gTriangleSoup);
    Bclean(data.gPosLife);
    Bclean(data.gNormals);
    Bclean(data.gForceLines);

    render.destroy();
}
