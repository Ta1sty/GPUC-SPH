#include "simulation_parameters.h"

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <random>

template <typename T>
T parseEnum(const YAML::Node &yaml, const std::string &key, std::initializer_list<std::pair<std::string, T>> mappings) {
    if (!yaml[key])
        return mappings.begin()->second;

    for (const auto& [k, v] : mappings) {
        if (k == yaml[key].as<std::string>())
            return v;
    }

    std::ostringstream oss;
    oss << "Invalid enum value for key \"" << key << "\": \"" << yaml[key].as<std::string>() << "\"";
    throw std::invalid_argument(oss.str());
}

template <typename T>
std::string dumpEnum(const T &value, std::initializer_list<std::pair<std::string, T>> mappings) {
    for (const auto& [k, v] : mappings)
        if (value == v)
            return k;

    throw std::invalid_argument("No mapping for enum value");
}

template <typename T>
T parse(const YAML::Node &yaml, const std::string &key, const T defaultValue) {
    if (!yaml[key])
        return defaultValue;

    return yaml[key].as<T>();
}

std::initializer_list<std::pair<std::string, SceneType>> sceneTypeMappings = {
        { "sph_box_2d", SceneType::SPH_BOX_2D }
};
std::initializer_list<std::pair<std::string, InitializationFunction>> initializationFunctionMappings = {
        { "uniform", InitializationFunction::UNIFORM },
        { "poisson_disk", InitializationFunction::POISSON_DISK }
};

SimulationParameters::SimulationParameters(const std::string &file) {
    YAML::Node yaml = YAML::LoadFile(file);

    type = parseEnum<SceneType>(yaml, "type", sceneTypeMappings);

    initializationFunction = parseEnum<InitializationFunction>(yaml, "initialization_function",
                                                               initializationFunctionMappings);

    numParticles = parse<uint32_t>(yaml, "num_particles", numParticles);
    std::random_device rd; // seed with TRNG if no seed is supplied
    randomSeed = parse<uint32_t>(yaml, "random_seed", rd());
}

std::string SimulationParameters::printToYaml() const {
    YAML::Node yaml;

    yaml["type"] = dumpEnum(type, sceneTypeMappings);
    yaml["initialization_function"] = dumpEnum(initializationFunction, initializationFunctionMappings);
    yaml["num_particles"] = numParticles;
    yaml["random_seed"] = randomSeed;

    return YAML::Dump(yaml);
}
