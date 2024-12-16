#include "project.h"
#include "host_timer.h"

ProjectSolution::ProjectSolution(AppResources& app, ProjectData& datad, uint32_t workGroupSize_x,
                                 uint32_t triangleCacheSize) :
    app(app), data(datad), workGroupSize_x(workGroupSize_x), triangleCacheSize(triangleCacheSize) {
    if (triangleCacheSize % 3 != 0)
        throw std::runtime_error("error: trangleCacheSize must be divisible by 3 or 0");

    // ########## Setup Integrate ############ 
    app.device.destroyShaderModule(particleShader);
    std::string compute = workingDir + "build/shaders/particles.comp.spv";

    Cmn::createShader(app.device, particleShader, compute);

    // ### Create Pipeline ###
    std::array<vk::SpecializationMapEntry, 3> specEntries = std::array<vk::SpecializationMapEntry, 3>{
        vk::SpecializationMapEntry{0U, 0U, sizeof(int)},
        vk::SpecializationMapEntry{1U, sizeof(int), sizeof(int)},
        vk::SpecializationMapEntry{2U, 2 * sizeof(int), sizeof(int)}
    };
    std::array<int, 3> specValues = {
        int(workGroupSize_x),
        int(data.particleCount),
        int(data.triangleCount)
    }; //for workgroup sizes

    vk::SpecializationInfo specInfo = vk::SpecializationInfo(CAST(specEntries), specEntries.data(),
                                                             CAST(specValues) * sizeof(int), specValues.data());
    // in case a pipeline was already created, destroy it
    app.device.destroyPipeline(particlePipeline);
    vk::PipelineShaderStageCreateInfo stageInfo(vk::PipelineShaderStageCreateFlags(),
                                                vk::ShaderStageFlagBits::eCompute, particleShader,
                                                "main", &specInfo);

    vk::ComputePipelineCreateInfo computeInfo(vk::PipelineCreateFlags(), stageInfo, data.layout);
    particlePipeline = app.device.createComputePipeline(nullptr, computeInfo, nullptr).value;

    vk::CommandBufferAllocateInfo allocInfo(
        app.computeCommandPool, vk::CommandBufferLevel::ePrimary, 1U);
    cb = app.device.allocateCommandBuffers(allocInfo)[0];
    setObjectName(app.device, cb, "particleComputeCommandBuffer");
}


void ProjectSolution::compute() {
    // If you want the delta T to depend on actual render time, you can use the following
    // you can also add a slowdown of speedup factor if needed
    //static HostTimer t;
    //data.push.dT = t.elapsed();
    //t.reset();
    uint32_t dx = (data.particleCount + workGroupSize_x - 1) / workGroupSize_x;

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cb.begin(beginInfo);
    cb.resetQueryPool(app.queryPool, 0, 2);
    cb.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, app.queryPool, 0);
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, particlePipeline);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, data.layout,
                          0U, 1U, &data.descriptorSet, 0U, nullptr);
    cb.pushConstants(data.layout, vk::ShaderStageFlagBits::eAll,
                     0, sizeof(ProjectData::PushConstant), &data.push);
    cb.dispatch(dx, 1, 1);
    cb.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, app.queryPool, 1);
    cb.end();

    // submit the command buffer to the queue and set up a fence.
    vk::SubmitInfo submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &cb); // submit a single command buffer
    vk::Fence fence = app.device.createFence(vk::FenceCreateInfo());
    // fence makes sure the control is not returned to CPU till command buffer is depleted

    app.computeQueue.submit({submitInfo}, fence);

    HostTimer timer;
    vk::Result haveIWaited = app.device.waitForFences({fence}, true, uint64_t(-1)); // wait for the fence indefinitely
    app.device.destroyFence(fence);

    mstime = timer.elapsed() * 1000;
}

std::vector<float> ProjectSolution::result() const {
    //std::cout<<w<<", "<<h<<std::endl;
    std::vector<float> resultPitched(1, 0.f);
    return resultPitched;
}

void ProjectSolution::cleanup() {
    app.device.destroyPipeline(particlePipeline);
    app.device.destroyShaderModule(particleShader);
    app.device.freeCommandBuffers(app.computeCommandPool, 1U, &cb);
}
