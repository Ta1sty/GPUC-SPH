#include "particle_physics.h"


ParticleSimulation::ParticleSimulation(const SimulationParameters &parameters) : simulationParameters(parameters) {
    // 0 for coordinates, 1 for velocities
    Cmn::addStorage(classResources.bindings, 0);
    Cmn::addStorage(classResources.bindings, 1);

    Cmn::createDescriptorSetLayout(resources.device, classResources.bindings, classResources.descriptorSetLayout);
    Cmn::createDescriptorPool(resources.device, classResources.bindings, classResources.descriptorPool);
    Cmn::allocateDescriptorSet(resources.device, classResources.descriptorSet, classResources.descriptorPool, classResources.descriptorSetLayout);

    vk::PushConstantRange pcr({vk::ShaderStageFlagBits::eCompute}, 0, sizeof(PushStruct));
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, classResources.descriptorSetLayout, pcr);

    classResources.pipelineLayout = resources.device.createPipelineLayout(pipelineLayoutInfo);

    vk::ShaderModule particleComputeSM;
    Cmn::createShader(resources.device, particleComputeSM, shaderPath("particle_simulation.comp"));

    std::array<vk::SpecializationMapEntry, 2> specEntries = std::array<vk::SpecializationMapEntry, 2> {
            vk::SpecializationMapEntry {0U, 0U, sizeof(workgroupSizeX)},
            vk::SpecializationMapEntry {1U, sizeof(workgroupSizeX), sizeof(workgroupSizeY)}};
    std::array<const uint32_t, 2> specValues = {workgroupSizeX, workgroupSizeY};
    vk::SpecializationInfo specInfo(specEntries, vk::ArrayProxyNoTemporaries<const uint32_t>(specValues));

    Cmn::createPipeline(resources.device, classResources.pipeline, classResources.pipelineLayout, specInfo, particleComputeSM);

    resources.device.destroyShaderModule(particleComputeSM);
}

void ParticleSimulation::updateCmd(const SimulationState &simulationState) {
    if (cmd == nullptr) {
        std::cout << "Command buffer is null, allocating new one" << std::endl;
        vk::CommandBufferAllocateInfo cmdInfo(resources.computeCommandPool, vk::CommandBufferLevel::ePrimary, 1);
        cmd = resources.device.allocateCommandBuffers(cmdInfo)[0];
    } else {
        std::cout << "Resetting command buffer" << std::endl;
        cmd.reset();
    }

    Cmn::bindBuffers(resources.device, simulationState.particleCoordinateBuffer.buf, classResources.descriptorSet, 0);
    Cmn::bindBuffers(resources.device, simulationState.particleVelocityBuffer.buf, classResources.descriptorSet, 1);
    uint32_t dx = (simulationState.parameters.numParticles + workgroupSizeX - 1) / workgroupSizeX;
    uint32_t dy = 1;// TODO : make this dynamic

    pushStruct.gravity = simulationState.parameters.gravity;
    pushStruct.deltaTime = simulationState.parameters.deltaTime;
    pushStruct.numParticles = simulationState.parameters.numParticles;
    pushStruct.collisionDamping = simulationState.parameters.collisionDampingFactor;

    cmd.begin(vk::CommandBufferBeginInfo());

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, classResources.pipelineLayout, 0, classResources.descriptorSet, {});
    cmd.pushConstants(classResources.pipelineLayout, {vk::ShaderStageFlagBits::eCompute}, 0, vk::ArrayProxy<const PushStruct>(pushStruct));

    // write into spatial-lookup and reset spatial-indices
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, classResources.pipeline);
    cmd.dispatch(dx, dy, 1);
    cmd.end();
}

vk::CommandBuffer ParticleSimulation::run(const SimulationState &simulationState) {
    if (nullptr == cmd) {
        std::cout << "ParticleSimulation::run called when cmd==NULL, updating the command buffer" << std::endl;
        updateCmd(simulationState);
    }

    return cmd;
}


ParticleSimulation::~ParticleSimulation() {
    resources.device.freeCommandBuffers(resources.computeCommandPool, cmd);
    classResources.destroy(resources.device);
}
