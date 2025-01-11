#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <iostream>
#include <cstdlib>

#include <vulkan/vulkan.hpp>
#include <vector>
#include "initialization.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <thread>

#include "project.h"

#include "renderdoc.h"
#include "render.h"
#include "simulation.h"

int width = 1200;
int height = 1000;

void render() {
    // Since our application is now frame-based, renderdoc can find frame delimiters on its own
    renderdoc::startCapture();

    Render render(resources, 2);
    SimulationParameters parameters { "../scenes/default.yaml" };
    Simulation simulation(parameters, render.camera);

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

        if (glfwGetKey(resources.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(resources.window, 1);

        if (glfwWindowShouldClose(resources.window))
            break;

        render.renderSimulationFrame(simulation);
    }
    resources.device.waitIdle();
}

int main() {
    try {
        AppResources app;

        initApp(app, true, "Project", width, height);
        renderdoc::initialize();

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
