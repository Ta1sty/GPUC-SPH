<<<<<<< HEAD
#include "helper.h"
#include <cstdlib>
#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "initialization.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <fstream>
=======
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
>>>>>>> main
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "project.h"

#include "render.h"
#include "renderdoc.h"
#include "simulation.h"

int width = 1200;
int height = 1000;

<<<<<<< HEAD

=======
>>>>>>> main
void render() {
    AppResources app;

    initApp(app, true, "Project", width, height);

    // Since our application is now frame-based, renderdoc can find frame delimiters on its own
    renderdoc::initialize();
    renderdoc::startCapture();

    Render render(app, 2);
<<<<<<< HEAD
    render.camera.position = glm::vec3(0.5, 2, 0.9);
    render.camera.phi = glm::pi<float>();
    render.camera.theta = 0.4 * glm::pi<float>();
    render.camera.aspect = (float) width / (float) height;
    //    Project project(app, render, 400000, workingDir + "Assets/cubeMonkey.obj");
    //    ProjectSolution solution(app, project.data, 192, 192);

    SimulationParameters parameters {"../scenes/default.yaml"};
    Simulation simulation(parameters);
=======

//    Project project(app, render, 400000, workingDir + "Assets/cubeMonkey.obj");
//    ProjectSolution solution(app, project.data, 192, 192);

    SimulationParameters parameters { "../scenes/default.yaml" };

    Simulation simulation(parameters, render.camera);
>>>>>>> main

    renderdoc::endCapture();

    // Loop until the user closes the window
    while (true) {
<<<<<<< HEAD
        //        double time = glfwGetTime();
        //        render.timedelta = time - render.prevtime;
        //        render.prevtime = time;
        //
        //        render.preInput();
=======
        double time = glfwGetTime();
        render.timedelta = time - render.prevtime;
        render.prevtime = time;

        render.preInput();
>>>>>>> main

        // Poll for and process events
        glfwPollEvents();

<<<<<<< HEAD
        //        render.input();
=======
        render.input();
>>>>>>> main

        if (glfwGetKey(app.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(app.window, 1);

        if (glfwWindowShouldClose(app.window))
            break;

        render.renderSimulationFrame(simulation);
<<<<<<< HEAD

        //        project.loop(solution);
=======
>>>>>>> main
    }

    app.device.waitIdle();

<<<<<<< HEAD
    //    solution.cleanup();
    //    project.cleanup();

    //    render.cleanup();
=======
    render.cleanup();
>>>>>>> main

    app.destroy();
}

int main() {
    try {
        render();
    } catch (vk::SystemError &err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        exit(-1);
    } catch (std::exception &err) {
        std::cout << "std::exception: " << err.what() << std::endl;
        exit(-1);
    } catch (...) {
        std::cout << "unknown error/n";
        exit(-1);
    }

    return EXIT_SUCCESS;
}
