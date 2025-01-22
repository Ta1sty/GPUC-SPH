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

    Cmn::createShader(resources.device, writeShader, shaderPath("spatial_lookup.write.comp"));
    Cmn::createShader(resources.device, sortShader, shaderPath("spatial_lookup.sort.bitonic.comp"));
    Cmn::createShader(resources.device, indexShader, shaderPath("spatial_lookup.index.comp"));
}

void SpatialLookup::createPipelines() {
    std::cout << "Spatial-Lookup-Build pipelines" << std::endl;

    resources.device.destroyPipeline(writePipeline);
    resources.device.destroyPipeline(sortPipeline);
    resources.device.destroyPipeline(indexPipeline);

    std::array<vk::SpecializationMapEntry, 1> specEntries {
            vk::SpecializationMapEntry(0, 0, sizeof(workgroupSizeX))};

    std::array<const uint32_t, 1> specValues = {
            workgroupSizeX};

    vk::SpecializationInfo specInfo(specEntries, vk::ArrayProxyNoTemporaries<const uint32_t>(specValues));

    Cmn::createPipeline(resources.device, writePipeline, pipelineLayout, specInfo, writeShader);
    Cmn::createPipeline(resources.device, sortPipeline, pipelineLayout, specInfo, sortShader);
    Cmn::createPipeline(resources.device, indexPipeline, pipelineLayout, specInfo, indexShader);
}

SpatialLookup::~SpatialLookup() {
    resources.device.destroyPipeline(writePipeline);
    resources.device.destroyPipeline(sortPipeline);
    resources.device.destroyPipeline(indexPipeline);

    resources.device.destroyShaderModule(writeShader);
    resources.device.destroyShaderModule(sortShader);
    resources.device.destroyShaderModule(indexShader);

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

    uint32_t sizeX = nextPowerOfTwo(std::min<uint32_t>(128, (state.parameters.numParticles + 1) / 2));
    if (sizeX != workgroupSizeX) {
        workgroupSizeX = sizeX;
        createPipelines();
    }

    SpatialLookupPushConstants pushConstants {
            state.spatialRadius,
            state.parameters.numParticles,
            nextPowerOfTwo(state.parameters.numParticles),
            0,
            0,
    };

    Cmn::bindBuffers(resources.device, state.spatialLookup.buf, descriptorSet, 0);
    Cmn::bindBuffers(resources.device, state.spatialIndices.buf, descriptorSet, 1);
    Cmn::bindBuffers(resources.device, state.particleCoordinateBuffer.buf, descriptorSet, 2);

    uint32_t dx = (state.parameters.numParticles + (workgroupSizeX * 2) - 1) / (workgroupSizeX * 2);

    std::cout
            << "Spatial-Lookup-Record"
            << " groupSize: " << workgroupSizeX
            << " groupCount: " << dx
            << " size: " << pushConstants.sort_n
            << " radius: " << pushConstants.cellSize << std::endl;

    cmd.begin(vk::CommandBufferBeginInfo());
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});

    // write into spatial-lookup and reset spatial-indices
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, writePipeline);
        cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
        cmd.dispatch(dx, 1, 1);
        computeBarrier(cmd);
    }

    uint32_t stepCounter = 0;
    uint32_t stepBreak = 1000;
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, sortPipeline);
        uint32_t n = pushConstants.sort_n;
        for (uint32_t k = 2; k <= pushConstants.sort_n; k *= 2) {
            pushConstants.sort_k = k;
            for (uint32_t j = k / 2; j > 0; j /= 2) {
                stepCounter++;
                pushConstants.sort_j = j;

                uint32_t blocks = (n + workgroupSizeX - 1) / workgroupSizeX;

                if (stepCounter > stepBreak) continue;

                //                std::cout << "n:" << pushConstants.sort_n << " ";
                //                std::cout << "k:" << pushConstants.sort_k << " ";
                //                std::cout << "j:" << pushConstants.sort_j << " ";
                //                std::cout << std::endl;


                cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
                cmd.dispatch(blocks, 1, 1);
                computeBarrier(cmd);
            }
        }
    }


    // write the start indices
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, indexPipeline);
    cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
    cmd.dispatch(dx, 1, 1);

    cmd.end();

    std::cout << "Spatial-lookup-Dispatches: " << (stepCounter) << std::endl;

    currentPushConstants = pushConstants;
}


vk::CommandBuffer SpatialLookup::run(SimulationState &state) {
    if (nullptr != cmd && state.spatialRadius == currentPushConstants.cellSize && state.parameters.numParticles == currentPushConstants.bufferSize) {
        return cmd;
    }

    updateCmd(state);

    return cmd;
}
