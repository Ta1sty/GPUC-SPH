#include <iostream>
#include <cstdlib>
#include <stb_image.h>
#include <stb_image_write.h>
#include "helper.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <vector>
#include "initialization.h"
#include "utils.h"
#include <GLFW/glfw3.h>

#include "project.h"

#include "renderdoc.h"
#include <render.h>
#include "sph_scene_parameters.h"
#include "sph_simulation_state.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

int width = 1200;
int height = 1000;

void render() {

    AppResources app;

    initApp(app, true, "Project", width, height);

    SPHSceneParameters parameters { "../scenes/default.yaml" };
    std::cout << "--- Scene parameters ---\n" << parameters.printToYaml() << "\n------------------------\n";
    SPHSimulationState simulationState {};
    simulationState.initialize(parameters, app);
    simulationState.cleanup(app);

    // Since our application is now frame-based, renderdoc can find frame delimiters on its own
    renderdoc::initialize();
    renderdoc::startCapture();

    Render render(app, 2);
    render.camera.position = glm::vec3(0.5, 2, 0.9);
    render.camera.phi = glm::pi<float>();
    render.camera.theta = 0.4 * glm::pi<float>();
    render.camera.aspect = (float)width/(float)height;
    Project project(app, render, 400000, workingDir + "Assets/cubeMonkey.obj");
    ProjectSolution solution(app, project.data, 192, 192);

    renderdoc::endCapture();

    // Loop until the user closes the window
    while (true) {
        double time = glfwGetTime();
        render.timedelta = time - render.prevtime;
        render.prevtime = time;

        render.preInput();

        // Poll for and process events
        glfwPollEvents();

        render.input();

        if (glfwGetKey(app.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(app.window, 1);

        if (glfwWindowShouldClose(app.window))
            break;

        // Render here //
        project.loop(solution);
    }

    app.device.waitIdle();

    solution.cleanup();
    project.cleanup();

    render.cleanup();

    app.destroy();
}

int main() {
    try {
        render();
    }
    catch (vk::SystemError& err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        exit(-1);
    }
    catch (std::exception& err) {
        std::cout << "std::exception: " << err.what() << std::endl;
        exit(-1);
    }
    catch (...) {
        std::cout << "unknown error/n";
        exit(-1);
    }
    return EXIT_SUCCESS;
}
