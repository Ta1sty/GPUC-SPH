#include "spatial_lookup.h"

SpatialLookup::SpatialLookup(const SimulationParameters &parameters) {

    descriptorBindings.clear();

    Cmn::addStorage(descriptorBindings, 0);
    Cmn::addStorage(descriptorBindings, 1);
    Cmn::addStorage(descriptorBindings, 2);

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
    resources.device.destroyPipeline(indexPipeline);
    indexPipeline = nullptr;

    resources.device.destroyShaderModule(writeShader);
    writeShader = nullptr;
    resources.device.destroyShaderModule(sortShader);
    sortShader = nullptr;
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
    Cmn::createShader(resources.device, indexShader, shaderPath("spatial_lookup.index.comp", type));

    Cmn::createPipeline(resources.device, writePipeline, pipelineLayout, specInfo, writeShader);
    Cmn::createPipeline(resources.device, sortPipeline, pipelineLayout, specInfo, sortShader);
    Cmn::createPipeline(resources.device, indexPipeline, pipelineLayout, specInfo, indexShader);
}

SpatialLookup::~SpatialLookup() {
    destroyPipelines();

    resources.device.destroyPipelineLayout(pipelineLayout);
    resources.device.destroyDescriptorPool(descriptorPool);
    resources.device.destroyDescriptorSetLayout(descriptorLayout);
}

void SpatialLookup::updateCmd(const SimulationState &state) {
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

    std::cout
            << "Spatial-Lookup-Record"
            << " size: " << workloadSize
            << " groupSize: " << workgroupSize
            << " groupCount: " << workgroupNum
            << " radius: " << pushConstants.cellSize << std::endl;

    cmd.begin(vk::CommandBufferBeginInfo());
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});

    // write into spatial-lookup and reset spatial-indices
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, writePipeline);
        cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
        cmd.dispatch(workgroupNum, 1, 1);
        computeBarrier(cmd);
    }

    uint32_t stepCounter = 0;
    uint32_t stepBreak = -1;
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, sortPipeline);
        uint32_t n = pushConstants.sort_n;
        for (uint32_t k = 2; k <= pushConstants.sort_n; k *= 2) {
            pushConstants.sort_k = k;
            for (uint32_t j = k / 2; j > 0; j /= 2) {
                stepCounter++;
                pushConstants.sort_j = j;

                if (stepCounter > stepBreak) continue;

                //                uint32_t gl_GlobalInvocationID = 5;
                //                uint32_t group_number = gl_GlobalInvocationID / j;
                //                uint32_t group_index = gl_GlobalInvocationID % j;
                //
                //                uint32_t i = 2 * group_number * j + group_index;
                //                uint32_t l = i ^ j;
                //
                //                std::cout << "n:" << pushConstants.sort_n << " ";
                //                std::cout << "k:" << pushConstants.sort_k << " ";
                //                std::cout << "j:" << pushConstants.sort_j << " ";
                //                std::cout << "t:" << gl_GlobalInvocationID << ":(" << i << "," << l << ") ";
                //                std::cout << std::endl;


                cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
                cmd.dispatch(workgroupNum, 1, 1);
                computeBarrier(cmd);
            }
        }
    }


    // write the start indices
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, indexPipeline);
    cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
    cmd.dispatch(workgroupNum, 1, 1);

    cmd.end();

    std::cout << "Spatial-lookup-Dispatches: " << (stepCounter + 2) << std::endl;

    currentPushConstants = pushConstants;
}


vk::CommandBuffer SpatialLookup::run(SimulationState &state) {
    if (nullptr != cmd &&
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
    groupSize = std::min<uint32_t>(128, size / 2);
    groupNum = size / 2 / groupSize;

    if (size == workgroupSize && parameters.type == static_cast<SceneType>(currentPushConstants.type)) return false;

    workloadSize = size;
    workgroupSize = groupSize;
    workgroupNum = groupNum;

    return true;
}
