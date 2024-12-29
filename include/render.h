#pragma once

#include <cmath>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include "task_common.h"
#include "imgui_ui.h"

class Camera {
public:
    glm::vec3 position = {4, 4, 4};
    float theta = glm::pi<float>() / 2, phi = glm::pi<float>() * 2.5 / 2;
    float fovy = 0.9, aspect = 1280.f / 720.f, near = 0.1, far = 100;

    glm::vec3 forwardDir() const {
        return {
            std::sin(phi) * std::sin(theta),
            std::cos(phi) * std::sin(theta),
            -std::cos(theta)
        };
    }

    glm::vec3 tangentDir() const {
        return {
            std::cos(phi),
            -std::sin(phi),
            0.f
        };
    }

    void rotateTheta(float radians) {
        theta = std::clamp(theta + radians, 0.f, glm::pi<float>());
    }

    void rotatePhi(float radians) {
        phi += radians;
        phi = std::fmod(phi, 2 * glm::pi<float>());
        if (phi < 0.f)
            phi += 2 * glm::pi<float>();
    }

    void moveInForwardDir(float distance) {
        position += forwardDir() * distance;
    }

    void moveInTangentDir(float distance) {
        position += tangentDir() * distance;
    }

    glm::mat4 viewMatrix() const {
        auto translationMatrix = glm::translate(glm::mat4(1.0), -position);
        auto rotationMatrixTheta = glm::rotate(glm::mat4(1.0), -theta, glm::vec3(1, 0, 0));
        auto rotationMatrixPhi = glm::rotate(glm::mat4(1.0), phi, glm::vec3(0, 0, 1));
        return rotationMatrixTheta * rotationMatrixPhi * translationMatrix;
    }

    glm::mat4 projectionMatrix() const {
        return glm::perspective(fovy, aspect, near, far);
    }

    glm::mat4 viewProjectionMatrix() const {
        return projectionMatrix() * viewMatrix();
    }
};

class Render {
public:
    struct PushConstant {
        glm::mat4 mvp;
    };

    Camera camera;
    uint64_t currentFrameIdx;
    double xdiff, ydiff;
    double prevxpos, prevypos;
    double prevtime;
    double timedelta;
    bool wp, ap, sp, dp;
    bool doRawMouseInput;
    bool doingRawMouseInput;

    AppResources& app;
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

    Render(AppResources& app, int framesinlight) : app(app), framesinlight(framesinlight) {
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

    void cleanup() {
        app.device.destroyPipelineLayout(layout);
        app.device.destroyRenderPass(renderPass);
        app.device.destroyPipeline(opaquePipeline);

        app.device.destroyImageView(depthImageView);
        app.device.freeMemory(depthImageMemory);
        app.device.destroyImage(depthImage);

        for (auto framebuffer : framebuffers)
            app.device.destroyFramebuffer(framebuffer);
        for (auto fence : fences)
            app.device.destroyFence(fence);
        for (auto semaphore : swapchainAcquireSemaphores)
            app.device.destroySemaphore(semaphore);
        for (auto semaphore : completionSemaphores)
            app.device.destroySemaphore(semaphore);
    }

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        Render* render = (Render*)glfwGetWindowUserPointer(window);
        if (render->doingRawMouseInput) {
            render->xdiff += xpos - render->prevxpos;
            render->ydiff += ypos - render->prevypos;
            render->prevxpos = xpos;
            render->prevypos = ypos;
        }
    }

    void preInput() {
        xdiff = 0;
        ydiff = 0;
    }

    void input() {
        doRawMouseInput |= glfwGetKey(app.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        if (doingRawMouseInput != doRawMouseInput) {
            if (doRawMouseInput) {
                glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwSetInputMode(app.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                glfwGetCursorPos(app.window, &prevxpos, &prevypos);
            }
            else {
                glfwSetInputMode(app.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glfwGetCursorPos(app.window, &prevxpos, &prevypos);
            }
            doingRawMouseInput = doRawMouseInput;
        }
        if (glfwGetKey(app.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera.rotatePhi(xdiff * timedelta);
            camera.rotateTheta(-ydiff * timedelta);
            if (glfwGetKey(app.window, GLFW_KEY_W) == GLFW_PRESS) camera.moveInForwardDir(timedelta);
            if (glfwGetKey(app.window, GLFW_KEY_A) == GLFW_PRESS) camera.moveInTangentDir(-timedelta);
            if (glfwGetKey(app.window, GLFW_KEY_S) == GLFW_PRESS) camera.moveInForwardDir(-timedelta);
            if (glfwGetKey(app.window, GLFW_KEY_D) == GLFW_PRESS) camera.moveInTangentDir(timedelta);
        }
        doRawMouseInput = false;
    }

    template <typename O, typename T, typename U>
    void renderFrame(O opaque, T transparent, U ui) {
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

        std::array<vk::CommandBuffer,2> buffers = {render_cb, ui_cb};
        std::vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo(swapchainAcquireSemaphores[idx], dstStages, buffers, completionSemaphores[idx]),
        };
        app.graphicsQueue.submit(submitInfos, fences[idx]);

        vk::Result vr = app.graphicsQueue.presentKHR({
            1, &completionSemaphores[idx], 1, &app.swapchain, &swapchainIndex
        });
    }
};
