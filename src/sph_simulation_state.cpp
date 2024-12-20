//
// Created by Benedikt Krimmel on 12/20/2024.
//

#include "sph_simulation_state.h"
#include "initialization.h"

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

void SPHSimulationState::initialize(const SPHSceneParameters &parameters, AppResources &app) {
    using BFlag = vk::BufferUsageFlagBits;
    auto makeDLocalBuffer = [&](vk::BufferUsageFlags usage, vk::DeviceSize size, const std::string &name) -> Buffer {
        Buffer b;
        createBuffer(app.pDevice, app.device, size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, name, b.buf,
                     b.mem);
        return b;
    };

    if (numParticles != parameters.numParticles || type != parameters.type) {
        if (numParticles != 0)
            cleanup(app);

        numParticles = parameters.numParticles;
        type = parameters.type;

        switch (type) {
            case SceneType::SPH_BOX_2D:
                coordinateBufferSize = 2 * numParticles;
                break;
        }

        particleCoordinates = makeDLocalBuffer(BFlag::eTransferDst | BFlag::eStorageBuffer,
                                               coordinateBufferSize * sizeof(float), "particleCoordinates");
        // TODO: create additional buffers?
    }

    random.seed(parameters.randomSeed);

    std::vector<float> values;

    switch (parameters.initializationFunction) {
        case InitializationFunction::UNIFORM:
            values = initUniform(type, numParticles, random);
            break;
        case InitializationFunction::POISSON_DISK:
            values = initPoissonDisk(type, numParticles, random);
            break;
    }

    fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue,
                                particleCoordinates, values);
}

void SPHSimulationState::cleanup(AppResources &app) {
    auto Bclean = [&](Buffer& b) {
        app.device.destroyBuffer(b.buf);
        app.device.freeMemory(b.mem);
    };

    Bclean(particleCoordinates);
    // TODO: cleanup additional buffers
}
