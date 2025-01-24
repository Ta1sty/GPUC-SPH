#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <cstdlib>
#include <iostream>

#include "initialization.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "project.h"

#include "render.h"
#include "renderdoc.h"
#include "simulation.h"

int width = 1200;
int height = 1000;

void render() {
    Render render(resources, 2);

    Simulation simulation(render.camera);

    // Loop until the user closes the window
    while (true) {
        double time = glfwGetTime();
        render.timedelta = time - render.prevtime;
        render.prevtime = time;

        render.preInput();

        // Poll for and process events
        glfwPollEvents();

        render.input();

        //if (glfwGetKey(resources.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        //    glfwSetWindowShouldClose(resources.window, 1);

        if (glfwWindowShouldClose(resources.window))
            break;

        render.renderSimulationFrame(simulation);
    }
    resources.device.waitIdle();
}

int main(int argc, char *argv[]) {
    std::cout << "ARGS:" << std::endl;
    resources.args.resize(argc);
    for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << std::endl;
        resources.args[i] = argv[i];
    }

    try {
        initApp(true, "Project", width, height);
        renderdoc::initialize();

        render();

        resources.destroy();
    } catch (vk::SystemError &err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        std::cin.get();
        exit(-1);
    } catch (std::exception &err) {
        std::cout << "std::exception: " << err.what() << std::endl;
        std::cin.get();
        exit(-1);
    } catch (...) {
        std::cout << "unknown error/n";
        std::cin.get();
        exit(-1);
    }

    std::cout << "PRESS ENTER TO EXIT" << std::endl;
    std::cin.get();
    return EXIT_SUCCESS;
}
