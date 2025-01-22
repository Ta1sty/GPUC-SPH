#include "particle_physics.h"


ParticleSimulation::ParticleSimulation(const SimulationParameters &parameters) : simulationParameters(parameters) {

    Cmn::addStorage(bindings, 0);// particle coordinates input
    Cmn::addStorage(bindings, 1);// particle velocities
    Cmn::addStorage(bindings, 2);// particle densities
    Cmn::addStorage(bindings, 3);// spatial lookup
    Cmn::addStorage(bindings, 4);// spatial indices
    Cmn::addStorage(bindings, 5);// particle coordinates copy output

    Cmn::createDescriptorSetLayout(resources.device, bindings, descriptorSetLayout);
    Cmn::createDescriptorPool(resources.device, bindings, descriptorPool);
    Cmn::allocateDescriptorSet(resources.device, descriptorSet, descriptorPool, descriptorSetLayout);

    vk::PushConstantRange pcr({vk::ShaderStageFlagBits::eCompute}, 0, sizeof(ParticleSimulationPushConstants));
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, descriptorSetLayout, pcr);

    pipelineLayout = resources.device.createPipelineLayout(pipelineLayoutInfo);

    vk::ShaderModule particleComputeSM;
    vk::ShaderModule densityComputeSM;
    Cmn::createShader(resources.device, particleComputeSM, shaderPath("particle_simulation.comp"));
    Cmn::createShader(resources.device, densityComputeSM, shaderPath("density_update.comp"));

    std::array<vk::SpecializationMapEntry, 2> specEntries = std::array<vk::SpecializationMapEntry, 2> {
            vk::SpecializationMapEntry {0U, 0U, sizeof(workgroupSizeX)},
            vk::SpecializationMapEntry {1U, sizeof(workgroupSizeX), sizeof(workgroupSizeY)}};
    std::array<const uint32_t, 2> specValues = {workgroupSizeX, workgroupSizeY};
    vk::SpecializationInfo specInfo(specEntries, vk::ArrayProxyNoTemporaries<const uint32_t>(specValues));

    Cmn::createPipeline(resources.device, computePipeline, pipelineLayout, specInfo, particleComputeSM);
    Cmn::createPipeline(resources.device, densityPipeline, pipelineLayout, specInfo, densityComputeSM);

    resources.device.destroyShaderModule(particleComputeSM);
    resources.device.destroyShaderModule(densityComputeSM);
}

void ParticleSimulation::updateCmd(const SimulationState &simulationState) {
    vk::ArrayProxy<const ParticleSimulationPushConstants> pcr;
    // set up copy buffers
    vk::DeviceSize coordinateBufferSize;
    switch (simulationState.parameters.type) {
        case SceneType::SPH_BOX_2D:
            coordinateBufferSize = sizeof(glm::vec2) * simulationState.parameters.numParticles;
            break;
    }

    particleCoordinateBufferCopy = createDeviceLocalBuffer("buffer-particles-copy", coordinateBufferSize, vk::BufferUsageFlagBits::eVertexBuffer);
    if (cmd == nullptr) {
        std::cout << "ParticleSimulation command buffer is null, allocating new one" << std::endl;
        vk::CommandBufferAllocateInfo cmdInfo(resources.computeCommandPool, vk::CommandBufferLevel::ePrimary, 1);
        cmd = resources.device.allocateCommandBuffers(cmdInfo)[0];
    } else {
        cmd.reset();
    }

    Cmn::bindBuffers(resources.device, simulationState.particleCoordinateBuffer.buf, descriptorSet, 0);
    Cmn::bindBuffers(resources.device, simulationState.particleVelocityBuffer.buf, descriptorSet, 1);
    Cmn::bindBuffers(resources.device, simulationState.particleDensityBuffer.buf, descriptorSet, 2);
    Cmn::bindBuffers(resources.device, simulationState.spatialLookup.buf, descriptorSet, 3);
    Cmn::bindBuffers(resources.device, simulationState.spatialIndices.buf, descriptorSet, 4);
    Cmn::bindBuffers(resources.device, particleCoordinateBufferCopy.buf, descriptorSet, 5);
    uint32_t dx = (simulationState.parameters.numParticles + workgroupSizeX - 1) / workgroupSizeX;
    uint32_t dy = 1;// TODO : make this dynamic

    ParticleSimulationPushConstants pushConstants;
    pushConstants.gravity = simulationState.parameters.gravity;
    pushConstants.deltaTime = simulationState.parameters.deltaTime;
    pushConstants.numParticles = simulationState.parameters.numParticles;
    pushConstants.collisionDamping = simulationState.parameters.collisionDampingFactor;
    pushConstants.spatialRadius = simulationState.spatialRadius;
    pushConstants.targetDensity = simulationState.parameters.targetDensity;
    pushConstants.pressureMultiplier = simulationState.parameters.pressureMultiplier;

    cmd.begin(vk::CommandBufferBeginInfo());

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});
    cmd.pushConstants(pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, (pcr = pushConstants));
    // compute densities
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, densityPipeline);
    cmd.dispatch(dx, dy, 1);
    computeBarrier(cmd);

    // compute physics
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    cmd.dispatch(dx, dy, 1);
    computeBarrier(cmd);
    // copy particle coordinates
    cmd.copyBuffer(particleCoordinateBufferCopy.buf, simulationState.particleCoordinateBuffer.buf, vk::BufferCopy(0, 0, simulationState.parameters.numParticles * sizeof(glm::vec2)));
    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            vk::MemoryBarrier(
                    vk::AccessFlagBits::eShaderWrite,
                    vk::AccessFlagBits::eTransferRead),
            nullptr,
            nullptr);

    cmd.end();
    currentPushConstants = pushConstants;
}

vk::CommandBuffer ParticleSimulation::run(const SimulationState &simulationState) {
    // TODO implement 3D!
    if (simulationState.parameters.type != SceneType::SPH_BOX_2D)
        return nullptr;

    if (nullptr == cmd || hasStateChanged(simulationState)) {
        updateCmd(simulationState);
    }
    return cmd;
}

bool ParticleSimulation::hasStateChanged(const SimulationState &state) {
    if (currentPushConstants.spatialRadius == state.spatialRadius &&
        currentPushConstants.gravity == state.parameters.gravity &&
        currentPushConstants.deltaTime == state.parameters.deltaTime &&
        currentPushConstants.numParticles == state.parameters.numParticles &&
        currentPushConstants.collisionDamping == state.parameters.collisionDampingFactor &&
        currentPushConstants.targetDensity == state.parameters.targetDensity &&
        currentPushConstants.pressureMultiplier == state.parameters.pressureMultiplier) {
        return false;
    } else {
        return true;
    }
}


ParticleSimulation::~ParticleSimulation() {

    resources.device.destroyPipeline(computePipeline);
    resources.device.destroyPipeline(densityPipeline);
    //PipelineLayout should be destroyed before DescriptorPool
    resources.device.destroyPipelineLayout(pipelineLayout);
    //DescriptorPool should be destroyed before the DescriptorSetLayout
    resources.device.destroyDescriptorPool(descriptorPool);
    resources.device.destroyDescriptorSetLayout(descriptorSetLayout);
}
