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

    double diff(Query start, Query end, double previous) {
        double t_start = timestamps.at(start);
        double t_end = timestamps.at(end);

        if (std::abs(t_start - t_end) < 0.0001) {
            return previous;
        }
        return (t_end - t_start) / 1000 / 1000;
    }

public:
    explicit QueryTimes(const std::map<Query, double> &timestamps, const QueryTimes &previous) : timestamps(timestamps) {
        if (timestamps.empty()) {
            return;
        }

        reset = diff(ResetBegin, ResetEnd, previous.reset);
        physics = diff(PhysicsBegin, PhysicsEnd, previous.physics);
        renderCompute = diff(RenderComputeBegin, RenderComputeEnd, previous.renderCompute);
        lookup = diff(LookupBegin, LookupEnd, previous.lookup);
        render = diff(RenderBegin, RenderEnd, previous.render);
        copy = diff(CopyBegin, CopyEnd, previous.copy);
        ui = diff(UiBegin, UiEnd, previous.ui);
        total = reset + physics + renderCompute + lookup + render + copy + ui;
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
