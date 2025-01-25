#pragma once

#include "camera.h"
#include "initialization.h"
#include <map>

struct SimulationParameters;
struct RenderParameters;
struct SimulationState;

struct QueryTimes {
private:
    std::map<Query, double> timestamps;

    double diff(Query start, Query end) {
        return (timestamps.at(end) - timestamps.at(start)) / 1000 / 1000;
    }

public:
    explicit QueryTimes(const std::map<Query, double> &timestamps, const QueryTimes &previous) : timestamps(timestamps) {
        if (timestamps.empty()) {
            return;
        }

        reset = diff(ResetBegin, ResetEnd);

        if (timestamps.at(PhysicsBegin) != 0)
            physics = diff(PhysicsBegin, PhysicsEnd);
        else
            physics = previous.physics;

        if (timestamps.at(RenderComputeBegin))
            renderCompute = diff(RenderComputeBegin, RenderComputeEnd);
        else
            renderCompute = 0.0f;


        lookup = diff(LookupBegin, LookupEnd);
        render = diff(RenderBegin, RenderEnd);
        copy = diff(CopyBegin, CopyEnd);
        ui = diff(UiBegin, UiEnd);
        total = reset + physics + lookup + render + copy + ui;
        fps = 1000 / total;
    }

    double fps = 0;
    double total = 0;
    double reset = 0;
    double physics = 0;
    double lookup = 0;
    double renderCompute = 0;
    double render = 0;
    double copy = 0;
    double ui = 0;
};

struct UpdateFlags {
    bool resetSimulation = false;
    bool togglePause = false;
    bool stepSimulation = false;
    bool runChecks = false;
    bool loadSceneFromFile = false;
    bool printRenderSettings = false;
};


/**
 * Wrap Parameters and update flags in convenience struct.
 */
struct UiBindings {
    uint32_t frameIndex;
    SimulationParameters &simulationParameters;
    RenderParameters &renderParameters;
    SimulationState *simulationState;
    QueryTimes queryTimes;
    UpdateFlags updateFlags;
    /**
     * Flags passed back to the simulation from the UI. Used to restart simulation, ...
     */

    /**
     * Wrap in constructor so you don't have to pass the update flags when initializing.
     */
    inline UiBindings(uint32_t frameIndex, SimulationParameters &simulationParameters,
                      RenderParameters &renderParameters, SimulationState *simulationState, QueryTimes queryTimes) : frameIndex(frameIndex),
                                                                                                                     simulationParameters(simulationParameters),
                                                                                                                     renderParameters(renderParameters),
                                                                                                                     simulationState(simulationState),
                                                                                                                     queryTimes(queryTimes),
                                                                                                                     updateFlags() {}
};

class ImguiUi {
    const std::string uiOffArg = "-ui-off";
    const std::string sceneArg = "-scene-";

    bool disabled;
    vk::DescriptorPool descriptorPool;
    vk::RenderPass renderPass;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Framebuffer> frameBuffers;
    std::vector<std::string> sceneFiles;
    std::vector<const char *> sceneFilesCStr;// sceneFiles[].c_str()
    int currentSceneFile = 0;


    void drawUi(UiBindings &bindings);
    void destroyCommandBuffers();
    void initCommandBuffers();

public:
    explicit ImguiUi();
    ~ImguiUi();

    vk::CommandBuffer updateCommandBuffer(uint32_t index, UiBindings &bindings);

    [[nodiscard]] std::string getSelectedSceneFile() const;
};
