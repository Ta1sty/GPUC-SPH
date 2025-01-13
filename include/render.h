#pragma once

#include <cmath>

#include "camera.h"
#include "imgui_ui.h"
#include "task_common.h"

class Simulation;

class Render {
public:
    struct PushConstant {
        glm::mat4 mvp;
    };

    std::shared_ptr<Camera> camera;
    uint64_t currentFrameIdx;
    double xdiff, ydiff;
    double prevxpos, prevypos;
    double prevtime;
    double timedelta;
    bool wp, ap, sp, dp;
    bool doRawMouseInput;
    bool doingRawMouseInput;

    AppResources &app;
    int framesinlight;

    vk::PipelineLayout layout;
    vk::RenderPass renderPass;
    vk::Pipeline opaquePipeline;

    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;
    std::vector<vk::Framebuffer> framebuffers;

    std::vector<vk::Fence> fences;
    std::vector<vk::Semaphore> swapchainAcquireSemaphores;
    std::vector<vk::Semaphore> completionSemaphores;
    std::vector<vk::CommandBuffer> commandBuffers;

    Render(AppResources &app, int framesinlight);
    ~Render();

    static void mouseCallback(GLFWwindow *window, double xpos, double ypos);

    void preInput();

    void input();

    void renderSimulationFrame(Simulation &simulation);

    template<typename O, typename T, typename U>
    void renderFrame(O opaque, T transparent, U ui);
};

template<typename O, typename T, typename U>
void Render::renderFrame(O opaque, T transparent, U ui) {
    auto idx = currentFrameIdx % framesinlight;
    if (app.device.waitForFences({fences[idx]}, true, ~0) != vk::Result::eSuccess)
        throw std::runtime_error("Waiting for fence didn't succeed!");
    app.device.resetFences({fences[idx]});
    auto result = app.device.acquireNextImageKHR(app.swapchain, ~0, swapchainAcquireSemaphores[idx], VK_NULL_HANDLE);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("Couldn't acquire next swapchain image!");
    auto swapchainIndex = result.value;

    vk::CommandBuffer render_cb = commandBuffers[idx];
    render_cb.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::ClearValue clearValues[2];
    clearValues[0].color.uint32 = {{0, 0, 0, 0}};
    clearValues[1].depthStencil.depth = 1;
    render_cb.beginRenderPass(
            {renderPass, framebuffers[swapchainIndex], {{0, 0}, app.extent}, 2, &clearValues[0]},
            vk::SubpassContents::eInline);
    opaque(render_cb);
    render_cb.nextSubpass(vk::SubpassContents::eInline);
    transparent(render_cb);
    render_cb.endRenderPass();
    render_cb.end();

    vk::CommandBuffer ui_cb = ui(swapchainIndex);

    vk::PipelineStageFlags dstStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    std::array<vk::CommandBuffer, 2> buffers = {render_cb, ui_cb};
    std::vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo(swapchainAcquireSemaphores[idx], dstStages, buffers, completionSemaphores[idx]),
    };
    app.graphicsQueue.submit(submitInfos, fences[idx]);

    vk::Result vr = app.graphicsQueue.presentKHR({1, &completionSemaphores[idx], 1, &app.swapchain, &swapchainIndex});
}
