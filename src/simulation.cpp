#include "simulation.h"
#include "renderer.h"
#include "particle_simulation.h"
#include "hashgrid.h"


std::vector<float> initUniform(SceneType sceneType, uint32_t numParticles, std::mt19937 &random) {
    std::vector<float> values;

    switch (sceneType) {
        case SceneType::SPH_BOX_2D:
            values.resize(2 * numParticles);
            std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
            for (auto &v : values) {
                v = distribution(random);
            }
    }

    return values;
}

std::vector<float> initPoissonDisk(SceneType sceneType, uint32_t numParticles, std::mt19937 &random) {
    throw std::exception("initPoissonDisk() is not implemented");

    std::vector<float> values;

    switch (sceneType) {
        case SceneType::SPH_BOX_2D:
            break;
    }
}

SimulationState::SimulationState(const SimulationParameters &parameters) {
    using BFlag = vk::BufferUsageFlagBits;
    auto makeDLocalBuffer = [&](vk::BufferUsageFlags usage, vk::DeviceSize size, const std::string &name) -> Buffer {
        Buffer b;
        createBuffer(resources.pDevice, resources.device, size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, name, b.buf,
                     b.mem);
        return b;
    };

    switch (parameters.type) {
        case SceneType::SPH_BOX_2D:
            coordinateBufferSize = 2 * parameters.numParticles;
            break;
    }

    particleCoordinates = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                           coordinateBufferSize * sizeof(float), "particleCoordinates");

    random.seed(parameters.randomSeed);

    std::vector<float> values;

    switch (parameters.initializationFunction) {
        case InitializationFunction::UNIFORM:
            values = initUniform(parameters.type, parameters.numParticles, random);
            break;
        case InitializationFunction::POISSON_DISK:
            values = initPoissonDisk(parameters.type, parameters.numParticles, random);
            break;
    }

    fillDeviceWithStagingBuffer(resources.pDevice, resources.device, resources.transferCommandPool, resources.transferQueue,
                                particleCoordinates, values);

}

SimulationState::~SimulationState() {
    auto Bclean = [&](Buffer& b) {
        resources.device.destroyBuffer(b.buf);
        resources.device.freeMemory(b.mem);
    };

    Bclean(particleCoordinates);
}

Simulation::Simulation() {

}

void Simulation::run() {

//    auto buf1 = particleSimulation->run();
//    auto buf2 = hashGrid->run();
//    auto buf3 = renderer->run();
//    auto buf4 = imguiUi.updateCommandBuffer(0, state.uiBindings);

    // submit with synchronization

    // copy rendered image (or debug image) to swapchain image

    // present swapchain image

}
