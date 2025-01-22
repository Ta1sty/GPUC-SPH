#include "simulation_parameters.h"

#include <random>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

template<typename T>
T parseEnum(const YAML::Node &yaml, const std::string &key, std::initializer_list<std::pair<std::string, T>> mappings) {
    if (!yaml[key])
        return mappings.begin()->second;

    for (const auto &[k, v]: mappings) {
        if (k == yaml[key].as<std::string>())
            return v;
    }

    std::ostringstream oss;
    oss << "Invalid enum value for key \"" << key << "\": \"" << yaml[key].as<std::string>() << "\"";
    throw std::invalid_argument(oss.str());
}

template<typename T>
std::string dumpEnum(const T &value, std::initializer_list<std::pair<std::string, T>> mappings) {
    for (const auto &[k, v]: mappings)
        if (value == v)
            return k;

    throw std::invalid_argument("No mapping for enum value");
}

template<typename T>
T parse(const YAML::Node &yaml, const std::string &key, const T defaultValue) {
    if (!yaml[key])
        return defaultValue;

    return yaml[key].as<T>();
}

const Mappings<SceneType> sceneTypeMappings {
        {"sph_box_2d", SceneType::SPH_BOX_2D},
        {"sph_box_3d", SceneType::SPH_BOX_3D}};
const Mappings<InitializationFunction> initializationFunctionMappings {
        {"uniform", InitializationFunction::UNIFORM},
        {"poisson_disk", InitializationFunction::POISSON_DISK}};
const Mappings<SelectedImage> selectedImageMappings {
        {"render", SelectedImage::RENDER},
        {"debug_physics", SelectedImage::DEBUG_PHYSICS},
        {"debug_sort", SelectedImage::DEBUG_SORT},
        {"debug_renderer", SelectedImage::DEBUG_RENDERER}};
const Mappings<RenderBackgroundField> renderBackgroundFieldMappings {
        {"cell_hash", RenderBackgroundField::CELL_HASH},
        {"density", RenderBackgroundField::DENSITY}};

SimulationParameters::SimulationParameters(const std::string &file) {
    YAML::Node yaml = YAML::LoadFile(file);

    type = parseEnum<SceneType>(yaml, "type", sceneTypeMappings);

    initializationFunction = parseEnum<InitializationFunction>(yaml, "initialization_function",
                                                               initializationFunctionMappings);

    numParticles = parse<uint32_t>(yaml, "num_particles", numParticles);
    std::random_device rd;// seed with TRNG if no seed is supplied
    randomSeed = parse<uint32_t>(yaml, "random_seed", rd());
    gravity = parse<float>(yaml, "gravity", gravity);
    deltaTime = parse<float>(yaml, "deltaTime", deltaTime);
    collisionDampingFactor = parse<float>(yaml, "collisionDampingFactor", collisionDampingFactor);
    targetDensity = parse<float>(yaml, "targetDensity", targetDensity);
    pressureMultiplier = parse<float>(yaml, "pressureMultiplier", pressureMultiplier);
    viscosity = parse<float>(yaml, "viscosity", viscosity);
}

std::string SimulationParameters::printToYaml() const {
    YAML::Node yaml;

    yaml["type"] = dumpEnum(type, sceneTypeMappings);
    yaml["initialization_function"] = dumpEnum(initializationFunction, initializationFunctionMappings);
    yaml["num_particles"] = numParticles;
    yaml["random_seed"] = randomSeed;
    yaml["gravity"] = gravity;
    yaml["delta_time"] = deltaTime;
    yaml["collision_damping_factor"] = collisionDampingFactor;

    return YAML::Dump(yaml);
}
