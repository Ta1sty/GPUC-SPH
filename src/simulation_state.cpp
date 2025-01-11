#include <utility>
#include <vector>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <memory>
#include "simulation_state.h"
#include "render.h"
#include "debug_image.h"

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
    throw std::runtime_error("not implemented");

    std::vector<float> values;

    switch (sceneType) {
        case SceneType::SPH_BOX_2D:
            break;
    }
}

SimulationState::SimulationState(const SimulationParameters &_parameters, std::shared_ptr<Camera> camera) : parameters(_parameters), random(parameters.randomSeed), camera(std::move(camera)) {
    std::cout << "------------- Initializing Simulation State -------------\n";
    std::cout << parameters.printToYaml() << std::endl;
    std::cout << "---------------------------------------------------------\n";

    debugImagePhysics = std::make_unique<DebugImage>("debug-image-particle");
    debugImageSort = std::make_unique<DebugImage>("debug-image-sort");
    debugImageRenderer = std::make_unique<DebugImage>("debug-image-render");

    vk::DeviceSize coordinateBufferSize;
    switch (parameters.type) {
        case SceneType::SPH_BOX_2D:
            coordinateBufferSize = sizeof(Particle) * parameters.numParticles;
            break;
    }

    particleCoordinateBuffer = createDeviceLocalBuffer("buffer-particles", coordinateBufferSize, vk::BufferUsageFlagBits::eVertexBuffer);
    std::vector<float> values;
    switch (parameters.initializationFunction) {
        case InitializationFunction::UNIFORM:
            values = initUniform(parameters.type, parameters.numParticles, random);
            break;
        case InitializationFunction::POISSON_DISK:
            values = initPoissonDisk(parameters.type, parameters.numParticles, random);
            break;
    }
    fillDeviceWithStagingBuffer(particleCoordinateBuffer, values);

    // Spatial Lookup
    spatialLookup = createDeviceLocalBuffer("spatialLookup", parameters.numParticles * sizeof (SpatialLookupEntry));
    spatialIndices = createDeviceLocalBuffer("startIndices", parameters.numParticles * sizeof(uint32_t));
}

SimulationState::~SimulationState() {
    // cleaning up all by itself via destructor magic ~ v ~
}

bool SimulationTime::advance(double add) {
    time += add;

    // it's time for a tick
    if (time > lastUpdate + tickRate){
        lastUpdate += tickRate;
        ticks++;
        return true;
    }

    return false;
}
