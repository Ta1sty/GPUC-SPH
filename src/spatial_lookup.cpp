#include "spatial_lookup.h"

SpatialLookup::SpatialLookup(const SimulationParameters &parameters) {

    descriptorBindings.clear();

    Cmn::addStorage(descriptorBindings, 0);
    Cmn::addStorage(descriptorBindings, 1);
    Cmn::addStorage(descriptorBindings, 2);
    Cmn::addStorage(descriptorBindings, 3);

    Cmn::createDescriptorSetLayout(resources.device, descriptorBindings, descriptorLayout);

    Cmn::createDescriptorPool(resources.device, descriptorBindings, descriptorPool);
    Cmn::allocateDescriptorSet(resources.device, descriptorSet, descriptorPool, descriptorLayout);

    vk::PushConstantRange pcr({vk::ShaderStageFlagBits::eCompute}, 0, sizeof(SpatialLookupPushConstants));
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, descriptorLayout, pcr);
    pipelineLayout = resources.device.createPipelineLayout(pipelineLayoutInfo);
}

void SpatialLookup::destroyPipelines() {
    resources.device.destroyPipeline(writePipeline);
    writePipeline = nullptr;
    resources.device.destroyPipeline(sortPipeline);
    sortPipeline = nullptr;
    resources.device.destroyPipeline(sortLocalPipeline);
    sortLocalPipeline = nullptr;
    resources.device.destroyPipeline(indexPipeline);
    indexPipeline = nullptr;

    resources.device.destroyShaderModule(writeShader);
    writeShader = nullptr;
    resources.device.destroyShaderModule(sortShader);
    sortShader = nullptr;
    resources.device.destroyShaderModule(sortLocalShader);
    sortLocalShader = nullptr;
    resources.device.destroyShaderModule(indexShader);
    indexShader = nullptr;
}

void SpatialLookup::createPipelines(SceneType type) {
    std::cout << "Spatial-Lookup-Build pipelines" << std::endl;

    std::array<vk::SpecializationMapEntry, 1> specEntries {
            vk::SpecializationMapEntry(0, 0, sizeof(workgroupSize))};
    std::array<const uint32_t, 1> specValues = {workgroupSize};

    vk::SpecializationInfo specInfo(specEntries, vk::ArrayProxyNoTemporaries<const uint32_t>(specValues));

    Cmn::createShader(resources.device, writeShader, shaderPath("spatial_lookup.write.comp", type));
    Cmn::createShader(resources.device, sortShader, shaderPath("spatial_lookup.sort.bitonic.comp", type));
    Cmn::createShader(resources.device, sortLocalShader, shaderPath("spatial_lookup.sort.bitonic.local.comp", type));
    Cmn::createShader(resources.device, indexShader, shaderPath("spatial_lookup.index.comp", type));

    Cmn::createPipeline(resources.device, writePipeline, pipelineLayout, specInfo, writeShader);
    Cmn::createPipeline(resources.device, sortPipeline, pipelineLayout, specInfo, sortShader);
    Cmn::createPipeline(resources.device, sortLocalPipeline, pipelineLayout, specInfo, sortLocalShader);
    Cmn::createPipeline(resources.device, indexPipeline, pipelineLayout, specInfo, indexShader);
}

SpatialLookup::~SpatialLookup() {
    destroyPipelines();

    resources.device.destroyPipelineLayout(pipelineLayout);
    resources.device.destroyDescriptorPool(descriptorPool);
    resources.device.destroyDescriptorSetLayout(descriptorLayout);
}

void SpatialLookup::updateCmd(const SimulationState &state) {
    useSharedMemory = state.spatialLocalSort;

    vk::ArrayProxy<const SpatialLookupPushConstants> pcr;

    if (nullptr == cmd) {
        vk::CommandBufferAllocateInfo cmdInfo(resources.computeCommandPool, vk::CommandBufferLevel::ePrimary, 1);
        cmd = resources.device.allocateCommandBuffers(cmdInfo)[0];
    } else {
        cmd.reset();
    }

    if (update(state.parameters)) {
        destroyPipelines();
        createPipelines(state.parameters.type);
    }

    SpatialLookupPushConstants pushConstants {
            static_cast<int>(state.parameters.type),
            state.spatialRadius,
            state.parameters.numParticles,
            workloadSize,
            0,
            0,
    };

    Cmn::bindBuffers(resources.device, state.spatialLookup.buf, descriptorSet, 0);
    Cmn::bindBuffers(resources.device, state.spatialIndices.buf, descriptorSet, 1);
    Cmn::bindBuffers(resources.device, state.particleCoordinateBuffer.buf, descriptorSet, 2);
    Cmn::bindBuffers(resources.device, state.spatialCache.buf, descriptorSet, 3);

    std::cout
            << "Spatial-Lookup-Record"
            << " size: " << workloadSize
            << " groupSize: " << workgroupSize
            << " groupCount: " << workgroupNum
            << " radius: " << pushConstants.cellSize << std::endl;

    cmd.begin(vk::CommandBufferBeginInfo());
    writeTimestamp(cmd, LookupBegin);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});

    uint32_t dispatchCounter = 0;
    // write into spatial-lookup and reset spatial-indices
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, writePipeline);
        cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
        cmd.dispatch(workgroupNum, 1, 1);
        computeBarrier(cmd);
        dispatchCounter++;
    }

    uint32_t k = 1;
    uint32_t j = 0;
    bool merge = false;

    while (true) {
        if (j == 0) {
            k *= 2;
            j = k / 2;
            if (k > pushConstants.sort_n) break;
        }

        pushConstants.sort_k = k;
        pushConstants.sort_j = j;

        bool local = pushConstants.sort_j <= workgroupSize;
        if (!useSharedMemory) local = false;

        if (!local) {
            merge = false;
        }

        //        std::cout << "n:" << pushConstants.sort_n << " ";
        //        std::cout << "k:" << pushConstants.sort_k << " ";
        //        std::cout << "j:" << pushConstants.sort_j << " ";
        //        std::cout << "t:" << (local ? (merge ? "local-merged" : "local-start") : "global") << " ";
        //        std::cout << std::endl;

        if (!merge) {
            if (local) {
                cmd.bindPipeline(vk::PipelineBindPoint::eCompute, sortLocalPipeline);
                cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
                cmd.dispatch(workgroupNum, 1, 1);
                computeBarrier(cmd);
                dispatchCounter++;
                merge = true;
            } else {
                cmd.bindPipeline(vk::PipelineBindPoint::eCompute, sortPipeline);
                cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
                cmd.dispatch(workgroupNum, 1, 1);
                computeBarrier(cmd);
                dispatchCounter++;
            }
        }

        j /= 2;
    }

    // write the start indices
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, indexPipeline);
    cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
    cmd.dispatch(workgroupNum, 1, 1);

    writeTimestamp(cmd, LookupEnd);
    cmd.end();

    std::cout << "Spatial-lookup-Dispatches: " << dispatchCounter << std::endl;

    currentPushConstants = pushConstants;
}


vk::CommandBuffer SpatialLookup::run(SimulationState &state) {
    if (nullptr != cmd &&
        state.spatialLocalSort == useSharedMemory &&
        state.spatialRadius == currentPushConstants.cellSize &&
        state.parameters.numParticles == currentPushConstants.numElements &&
        state.parameters.type == static_cast<SceneType>(currentPushConstants.type)) {
        return cmd;
    }

    updateCmd(state);

    return cmd;
}
bool SpatialLookup::update(const SimulationParameters &parameters) {
    uint32_t size, groupSize, groupNum;

    size = nextPowerOfTwo(parameters.numParticles);
    groupSize = std::min<uint32_t>(1024, size / 2);
    groupNum = size / 2 / groupSize;

    if (size == workgroupSize && parameters.type == static_cast<SceneType>(currentPushConstants.type)) return false;

    workloadSize = size;
    workgroupSize = groupSize;
    workgroupNum = groupNum;

    return true;
}
