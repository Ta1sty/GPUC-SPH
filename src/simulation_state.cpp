#include "simulation_state.h"
#include "debug_image.h"
#include "render.h"
#include <cstdint>
#include <memory>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

std::vector<float> initUniform(SceneType sceneType, uint32_t numParticles, std::mt19937 &random) {
    std::vector<float> values;
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

    switch (sceneType) {
        case SceneType::SPH_BOX_2D:
            values.resize(2 * numParticles);
            for (auto &v: values) {
                v = distribution(random);
            }
            break;
        case SceneType::SPH_BOX_3D:
            values.resize(4 * numParticles);
            for (size_t i = 0; i < numParticles; i++) {
                for (size_t j = 0; j < 3; j++)
                    values[4 * i + j] = distribution(random);

                values[4 * i + 3] = -FLT_MAX;
            }
            break;
        default:
            throw std::runtime_error("uniform distribution cannot be created for this scene type");
            break;
    }

    return values;
}

std::vector<float> initPoissonDisk(SceneType sceneType, uint32_t numParticles, std::mt19937 &random) {
    throw std::runtime_error("poisson disk init not implemented");

    std::vector<float> values;

    switch (sceneType) {
        case SceneType::SPH_BOX_2D:
            break;
    }
}

std::vector<float> initJittered(SceneType sceneType, uint32_t numParticles, std::mt19937 &random) {
    std::vector<float> values;
    if (sceneType == SceneType::SPH_BOX_2D) {
        // Compute grid resolution.
        int gridSize = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(numParticles))));
        float cellSize = 1.0f / gridSize;
        values.reserve(2 * numParticles);
        std::uniform_real_distribution<float> offsetDist(0.0f, cellSize);
        int count = 0;
        // Iterate over a grid and jitter within each cell.
        for (int i = 0; i < gridSize && count < numParticles; i++) {
            for (int j = 0; j < gridSize && count < numParticles; j++) {
                float x = i * cellSize + offsetDist(random);
                float y = j * cellSize + offsetDist(random);
                values.push_back(x);
                values.push_back(y);
                count++;
            }
        }
    } else if (sceneType == SceneType::SPH_BOX_3D) {
        // Compute grid resolution.
        int gridSize = static_cast<int>(std::ceil(std::cbrt(static_cast<float>(numParticles))));
        float cellSize = 1.0f / gridSize;
        values.reserve(4 * numParticles);
        std::uniform_real_distribution<float> offsetDist(0.0f, cellSize);
        int count = 0;
        // Iterate over a 3D grid and jitter within each cell.
        for (int i = 0; i < gridSize && count < numParticles; i++) {
            for (int j = 0; j < gridSize && count < numParticles; j++) {
                for (int k = 0; k < gridSize && count < numParticles; k++) {
                    float x = i * cellSize + offsetDist(random);
                    float y = j * cellSize + offsetDist(random);
                    float z = k * cellSize + offsetDist(random);
                    values.push_back(x);
                    values.push_back(y);
                    values.push_back(z);
                    // The fourth component is reserved; keep as -FLT_MAX.
                    values.push_back(-FLT_MAX);
                    count++;
                }
            }
        }
    } else {
        throw std::runtime_error("Jittered initialization not implemented for this scene type");
    }

    return values;
}

SimulationState::SimulationState(const SimulationParameters &_parameters, std::shared_ptr<Camera> _camera)
    : parameters(_parameters), spatialRadius(_parameters.spatialRadius), random(parameters.randomSeed), camera(std::move(_camera)) {
    std::cout << "------------- Initializing Simulation State -------------\n";
    std::cout << parameters.printToYaml() << std::endl;
    std::cout << "---------------------------------------------------------\n";

    debugImagePhysics = std::make_unique<DebugImage>("debug-image-particle");
    debugImageSort = std::make_unique<DebugImage>("debug-image-sort");
    debugImageRenderer = std::make_unique<DebugImage>("debug-image-render");

    vk::DeviceSize coordinateBufferSize = 0;
    switch (parameters.type) {
        case SceneType::SPH_BOX_2D:
            coordinateBufferSize = sizeof(glm::vec2) * parameters.numParticles;

            camera->position = {0.5, 0.5,
                                0.5 / glm::tan(camera->fovy / 2.0f)};
            camera->phi = glm::pi<float>();
            camera->theta = 0.0f;
            break;
        case SceneType::SPH_BOX_3D:
            coordinateBufferSize = sizeof(glm::vec4) * parameters.numParticles;
            camera->reset();
            break;
        default:
            throw std::runtime_error("SimulationState cannot be initialized for this scene type");
            break;
    }
    // Particles
    particleCoordinateBuffer = createDeviceLocalBuffer("buffer-particles", coordinateBufferSize, vk::BufferUsageFlagBits::eVertexBuffer);
    particleVelocityBuffer = createDeviceLocalBuffer("buffer-velocities", coordinateBufferSize);
    particleDensityBuffer = createDeviceLocalBuffer("buffer-densities", parameters.numParticles * sizeof(float));
    std::vector<float> coordinateValues;
    std::vector<float> velocityValues(parameters.numParticles, 0.0f);
    std::vector<float> densityValues(parameters.numParticles, 0.0f);// initialize densities to 0

    switch (parameters.initializationFunction) {
        case InitializationFunction::UNIFORM:
            coordinateValues = initUniform(parameters.type, parameters.numParticles, random);
            break;
        case InitializationFunction::POISSON_DISK:
            coordinateValues = initPoissonDisk(parameters.type, parameters.numParticles, random);
            break;
        case InitializationFunction::JITTERED:
            coordinateValues = initJittered(parameters.type, parameters.numParticles, random);
            break;
    }

    fillDeviceWithStagingBuffer(particleCoordinateBuffer, coordinateValues);
    fillDeviceWithStagingBuffer(particleVelocityBuffer, velocityValues);
    fillDeviceWithStagingBuffer(particleDensityBuffer, densityValues);


    // Spatial Lookup
    uint32_t lookupSize = nextPowerOfTwo(parameters.numParticles);
    spatialLookup = createDeviceLocalBuffer("spatialLookup", lookupSize * sizeof(SpatialLookupEntry));
    spatialIndices = createDeviceLocalBuffer("spatialIndices", lookupSize * sizeof(SpatialIndexEntry));
    spatialCache = createDeviceLocalBuffer("spatialCache", lookupSize * sizeof(SpatialCacheEntry));

    // precomputed render stuff
    densityGrid = createDeviceLocalBuffer("density-grid", 256 * 256 * 256 * sizeof(float));
}

SimulationState::~SimulationState() {
    // cleaning up all by itself via destructor magic ~ v ~
}

void SimulationTime::pause() {
    lastUpdate = time;
}

bool SimulationTime::advance(double add) {
    time += add;
    frames++;

    // it's time for a tick
    if (time >= lastUpdate + tickRate) {
        lastUpdate += tickRate;
        ticks++;
        return true;
    }

    return false;
}
