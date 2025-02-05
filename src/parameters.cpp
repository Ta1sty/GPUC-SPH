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
        {"none", RenderBackgroundField::NONE},
        {"cell_key", RenderBackgroundField::CELL_HASH},
        {"cell_class", RenderBackgroundField::CELL_CLASS},
        {"density", RenderBackgroundField::DENSITY},
        {"velocity", RenderBackgroundField::VELOCITY},
        {"water", RenderBackgroundField::WATER}};
const Mappings<RenderParticleColor> renderParticleColorMappings {
        {"white", RenderParticleColor::WHITE},
        {"none", RenderParticleColor::NONE},
        {"num_neighbours", RenderParticleColor::NUM_NEIGHBOURS},
        {"density", RenderParticleColor::DENSITY},
        {"velocity", RenderParticleColor::VELOCITY}};

SimulationParameters::SimulationParameters(const YAML::Node &yaml) {
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
    spatialRadius = parse<float>(yaml, "spatial_radius", spatialRadius);
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
    yaml["spatial_radius"] = spatialRadius;

    return YAML::Dump(yaml);
}

RenderParameters::RenderParameters(const YAML::Node &yaml) {
    selectedImage = parseEnum<SelectedImage>(yaml, "selected_image", selectedImageMappings);
    backgroundEnvironment = parse<bool>(yaml, "background_environment", backgroundEnvironment);
    backgroundField = parseEnum<RenderBackgroundField>(yaml, "background_field", renderBackgroundFieldMappings);
    particleColor = parseEnum<RenderParticleColor>(yaml, "particle_color", renderParticleColorMappings);
    particleRadius = parse<float>(yaml, "particle_radius", particleRadius);
    densityGridShader = parse<std::string>(yaml, "density_grid_shader", densityGridShader);
    if (yaml["density_grid_wg_size"]) {
        auto &y = yaml["density_grid_wg_size"];
        densityGridWGSize = {
                y[0].as<uint32_t>(),
                y[1].as<uint32_t>(),
                y[2].as<uint32_t>()};
    }
}

std::string RenderParameters::printToYaml() const {
    YAML::Node yaml;

    yaml["selected_image"] = dumpEnum(selectedImage, selectedImageMappings);
    yaml["background_environment"] = backgroundEnvironment;
    yaml["background_field"] = dumpEnum(backgroundField, renderBackgroundFieldMappings);
    yaml["particle_color"] = dumpEnum(particleColor, renderParticleColorMappings);
    yaml["particle_radius"] = particleRadius;

    return YAML::Dump(yaml);
}

std::pair<RenderParameters, SimulationParameters> SceneParameters::loadParametersFromFile(const std::string sceneFile) {
    YAML::Node yaml = YAML::LoadFile(sceneFile);
    RenderParameters renderParameters;
    SimulationParameters simulationParameters;

    if (yaml["render"])
        renderParameters = RenderParameters(yaml["render"]);

    if (yaml["simulation"])
        simulationParameters = SimulationParameters(yaml["simulation"]);

    return {renderParameters, simulationParameters};
}
