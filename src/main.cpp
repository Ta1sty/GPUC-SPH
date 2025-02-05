#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <cstdlib>
#include <ctime>
#include <filesystem>
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

void benchmark() {
    const std::array<std::string, 16> benchmarkScenes {
            {"3d_8k_8x8x8.yaml",
             "3d_8k_8x8x8_naive.yaml",
             "3d_16k_8x8x8.yaml",
             "3d_16k_8x8x8_naive.yaml",
             "3d_32k_8x8x8.yaml",
             "3d_32k_8x8x8_naive.yaml",
             "3d_64k_16x8x8.yaml",
             "3d_64k_4x4x4.yaml",
             "3d_64k_8x4x4.yaml",
             "3d_64k_8x8x4.yaml",
             "3d_64k_8x8x8.yaml",
             "3d_64k_8x8x8_naive.yaml",
             "3d_128k_8x8x8.yaml",
             "3d_128k_8x8x8_naive.yaml",
             "3d_256k_8x8x8.yaml",
             "3d_512k_8x8x8.yaml"}};
    constexpr size_t BENCHMARK_NUM_FRAMES = 256;

    const auto t = std::time(nullptr);
    const auto tm = *std::localtime(&t);
    char _folderName[256];
    std::strftime(_folderName, 256, "benchmark_%y-%m-%d_%H-%M", &tm);
    std::string folderName {_folderName};
    if (!std::filesystem::exists(folderName))
        std::filesystem::create_directory(folderName);

    if (!std::filesystem::is_directory(folderName))
        throw std::runtime_error(folderName + " is not a valid directory");

    folderName.append("/");

    Render render(resources, 2);
    for (auto &sceneFile: benchmarkScenes) {
        std::ofstream f {folderName + sceneFile.substr(0, sceneFile.find('.')) + ".csv"};
        f << "reset,physics,lookup,render_compute,render,copy,ui,\n";
        auto w = [&](const double &v) {
            f << v << ",";
        };


        Simulation simulation {render.camera, "../scenes_benchmark/" + sceneFile};
        simulation.getState().paused = false;

        auto writeCSVRow = [&]() {
            auto &qt = simulation.getQueryTimes();
            w(qt.reset);
            w(qt.physics);
            w(qt.lookup);
            w(qt.renderCompute);
            w(qt.render);
            w(qt.copy);
            w(qt.ui);
            f << '\n';
        };

        for (size_t i = 0; i < BENCHMARK_NUM_FRAMES; i++) {
            double time = glfwGetTime();
            render.timedelta = time - render.prevtime;
            render.prevtime = time;

            render.preInput();

            // Poll for and process events
            glfwPollEvents();

            //render.input();

            if (glfwWindowShouldClose(resources.window))
                break;

            render.renderSimulationFrame(simulation);
            writeCSVRow();
        }

        f.close();
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

    //try {
    initApp(true, "Project", width, height);
    renderdoc::initialize();

    if (argc > 1 && argv[1] == std::string("benchmark")) {
        benchmark();
    } else {
        render();
    }

    resources.destroy();
    //} catch (vk::SystemError &err) {
    //     std::cout << "vk::SystemError: " << err.what() << std::endl;
    //     pauseExecution();
    //     exit(-1);
    // } catch (std::exception &err) {
    //     std::cout << "std::exception: " << err.what() << std::endl;
    //     pauseExecution();
    //     exit(-1);
    // } catch (...) {
    //     std::cout << "unknown error/n";
    //     pauseExecution();
    //     exit(-1);
    // }

    pauseExecution();
    return EXIT_SUCCESS;
}
