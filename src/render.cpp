#include "render.h"

#include "simulation.h"

void Render::renderSimulationFrame(Simulation &simulation) {
    auto idx = currentFrameIdx % framesinlight;
    if (app.device.waitForFences({fences[idx]}, true, ~0) != vk::Result::eSuccess)
        throw std::runtime_error("Waiting for fence didn't succeed!");
    app.device.resetFences({fences[idx]});
    auto result = app.device.acquireNextImageKHR(app.swapchain, ~0, swapchainAcquireSemaphores[idx], VK_NULL_HANDLE);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("Couldn't acquire next swapchain image!");

    auto swapchainIndex = result.value;

    simulation.run(swapchainIndex, swapchainAcquireSemaphores[idx], completionSemaphores[idx], fences[idx]);

    vk::PresentInfoKHR presentInfo(completionSemaphores[idx], app.swapchain, swapchainIndex);
    vk::detail::resultCheck(app.graphicsQueue.presentKHR(presentInfo), "Failed to present image");

    currentFrameIdx = (currentFrameIdx + 1) % framesinlight;
}

void Render::input() {
    doRawMouseInput |= glfwGetKey(app.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    if (doingRawMouseInput != doRawMouseInput) {
        if (doRawMouseInput) {
            glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(app.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            glfwGetCursorPos(app.window, &prevxpos, &prevypos);
        } else {
            glfwSetInputMode(app.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwGetCursorPos(app.window, &prevxpos, &prevypos);
        }
        doingRawMouseInput = doRawMouseInput;
    }
    if (glfwGetKey(app.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera->rotatePhi(xdiff * timedelta);
        camera->rotateTheta(-ydiff * timedelta);
        if (glfwGetKey(app.window, GLFW_KEY_W) == GLFW_PRESS) camera->moveInForwardDir(timedelta);
        if (glfwGetKey(app.window, GLFW_KEY_A) == GLFW_PRESS) camera->moveInTangentDir(-timedelta);
        if (glfwGetKey(app.window, GLFW_KEY_S) == GLFW_PRESS) camera->moveInForwardDir(-timedelta);
        if (glfwGetKey(app.window, GLFW_KEY_D) == GLFW_PRESS) camera->moveInTangentDir(timedelta);
    }
    doRawMouseInput = false;
}

void Render::preInput() {
    xdiff = 0;
    ydiff = 0;
}

void Render::mouseCallback(GLFWwindow *window, double xpos, double ypos) {
    Render *render = (Render *) glfwGetWindowUserPointer(window);
    if (render->doingRawMouseInput) {
        render->xdiff += xpos - render->prevxpos;
        render->ydiff += ypos - render->prevypos;
        render->prevxpos = xpos;
        render->prevypos = ypos;
    }
}

Render::~Render() {
    app.device.destroyPipelineLayout(layout);
    app.device.destroyRenderPass(renderPass);
    app.device.destroyPipeline(opaquePipeline);

    app.device.destroyImageView(depthImageView);
    app.device.freeMemory(depthImageMemory);
    app.device.destroyImage(depthImage);

    for (auto framebuffer: framebuffers)
        app.device.destroyFramebuffer(framebuffer);
    for (auto fence: fences)
        app.device.destroyFence(fence);
    for (auto semaphore: swapchainAcquireSemaphores)
        app.device.destroySemaphore(semaphore);
    for (auto semaphore: completionSemaphores)
        app.device.destroySemaphore(semaphore);
}

Render::Render(AppResources &app, int framesinlight) : app(app), framesinlight(framesinlight) {
    camera = std::make_shared<Camera>();

    glfwSetWindowUserPointer(app.window, this);
    glfwSetCursorPosCallback(app.window, &Render::mouseCallback);

    prevtime = glfwGetTime();

    currentFrameIdx = 0;

    {
        vk::PushConstantRange pushConstantRange = {
                vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(PushConstant)
        };

        vk::PipelineLayoutCreateInfo cI = {
                {}, 0, nullptr, 1, &pushConstantRange
        };

        layout = app.device.createPipelineLayout(cI, nullptr);
    }

    {
        vk::AttachmentDescription attachments[] = {
                {
                        vk::AttachmentDescriptionFlagBits(),
                        app.surfaceFormat.format,
                        vk::SampleCountFlagBits::e1,
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eStore,
                        vk::AttachmentLoadOp::eDontCare,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::ePresentSrcKHR,
                },
                {
                        vk::AttachmentDescriptionFlagBits(),
                        // TODO check if lower precision suffices
                        vk::Format::eD32Sfloat,
                        vk::SampleCountFlagBits::e1,
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::AttachmentLoadOp::eDontCare,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eDepthStencilAttachmentOptimal,

                }
        };
        vk::AttachmentReference attachmentReferences[] = {
                {
                        0, vk::ImageLayout::eColorAttachmentOptimal,
                }
        };
        vk::AttachmentReference depthStencilAttachmentRef = {
                1, vk::ImageLayout::eDepthStencilAttachmentOptimal
        };
        vk::SubpassDescription subpasses[] = {
                {
                        vk::SubpassDescriptionFlagBits(),
                        vk::PipelineBindPoint::eGraphics,
                        0, nullptr,
                        1, &attachmentReferences[0],
                        nullptr,
                        &depthStencilAttachmentRef,
                        0, nullptr
                },
                {
                        vk::SubpassDescriptionFlagBits(),
                        vk::PipelineBindPoint::eGraphics,
                        0, nullptr,
                        1, &attachmentReferences[0],
                        nullptr,
                        &depthStencilAttachmentRef,
                        0, nullptr
                }
        };

        vk::SubpassDependency dependencies = {
                0, 1, vk::PipelineStageFlagBits::eAllGraphics, vk::PipelineStageFlagBits::eAllGraphics,
                vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite
        };

        vk::RenderPassCreateInfo cI = {
                {}, 2, &attachments[0], 2, &subpasses[0], 1, &dependencies
        };

        renderPass = app.device.createRenderPass(cI);
    }
    {
        vk::ImageCreateInfo cI = {
                {}, vk::ImageType::e2D, vk::Format::eD32Sfloat, {app.extent.width, app.extent.height, 1},
                1, 1, vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::SharingMode::eExclusive, 0, {}, vk::ImageLayout::eUndefined
        };

        createImage(app.pDevice, app.device, cI, vk::MemoryPropertyFlagBits::eDeviceLocal, "Depth Buffer",
                    depthImage, depthImageMemory);

        depthImageView = app.device.createImageView({
                                                            {}, depthImage, vk::ImageViewType::e2D, vk::Format::eD32Sfloat,
                                                            {
                                                                    vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
                                                                    vk::ComponentSwizzle::eIdentity
                                                            },
                                                            {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
                                                    });
    }

    {
        framebuffers.clear();
        for (int i = 0; i < app.swapchainImages.size(); i++) {
            vk::ImageView attachments[] = {app.swapchainImageViews[i], depthImageView};
            framebuffers.push_back(app.device.createFramebuffer({
                                                                        {}, renderPass, 2, &attachments[0], app.extent.width, app.extent.height, 1
                                                                }));
        }
    }

    for (int i = 0; i < framesinlight; i++) {
        fences.push_back(app.device.createFence({vk::FenceCreateFlagBits::eSignaled}));
        swapchainAcquireSemaphores.push_back(app.device.createSemaphore({}));
        completionSemaphores.push_back(app.device.createSemaphore({}));
        commandBuffers.push_back(app.device.allocateCommandBuffers({
                                                                           app.graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1U
                                                                   })[0]);
    }
}


