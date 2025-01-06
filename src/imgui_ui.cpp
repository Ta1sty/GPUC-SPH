#include "imgui_ui.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "simulation_parameters.h"


ImguiUi::ImguiUi() {
    const vk::DescriptorPoolSize pool_sizes[] =
            {
                    {vk::DescriptorType::eSampler,       10},
                    {vk::DescriptorType::eUniformBuffer, 10},
                    {vk::DescriptorType::eStorageBuffer, 10},
            };

    vk::DescriptorPoolCreateInfo poolInfo({vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet}, 10, pool_sizes);

    descriptorPool = resources.device.createDescriptorPool(poolInfo);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    // Setup Platform/Renderer bindings

    vk::AttachmentDescription attachment(
            {},
            resources.surfaceFormat.format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::ePresentSrcKHR
    );

    vk::AttachmentReference color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription ui_subpass(
            {},
            vk::PipelineBindPoint::eGraphics,
            {},
            color_attachment,
            {},
            nullptr,
            {}
    );

    vk::SubpassDependency externalDependency(
            VK_SUBPASS_EXTERNAL,
            0,
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::AccessFlagBits::eColorAttachmentWrite},
            {vk::AccessFlagBits::eColorAttachmentWrite}
    );

    vk::RenderPassCreateInfo info({}, attachment, ui_subpass, externalDependency);
    renderPass = resources.device.createRenderPass(info);

    ImGui_ImplGlfw_InitForVulkan(resources.window, true);

    ImGui_ImplVulkan_InitInfo init_info = {
            resources.instance,
            resources.pDevice,
            resources.device,
            resources.gQ,
            resources.graphicsQueue,
            nullptr,
            descriptorPool,
            0,
            static_cast<uint32_t>(resources.swapchainImages.size()),
            static_cast<uint32_t>(resources.swapchainImages.size()),
            VK_SAMPLE_COUNT_1_BIT,
            false,
            {},
            nullptr,
            nullptr,
            0
    };

    ImGui_ImplVulkan_Init(&init_info, renderPass);
    ImGui::StyleColorsDark();

    ImGui_ImplVulkan_CreateFontsTexture();
}

void ImguiUi::initCommandBuffers() {

    if (!frameBuffers.empty() || commandPool || !commandBuffers.empty()) {
        destroyCommandBuffers();
    }

    vk::CommandPoolCreateInfo poolInfo({vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, resources.gQ);
    commandPool = resources.device.createCommandPool(poolInfo);

    vk::CommandBufferAllocateInfo allocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, resources.swapchainImages.size());
    commandBuffers = resources.device.allocateCommandBuffers(allocateInfo);

    for (uint32_t i = 0; i < resources.swapchainImages.size(); i++) {
        vk::FramebufferCreateInfo info(
                {},
                renderPass,
                resources.swapchainImageViews[i],
                resources.extent.width,
                resources.extent.height,
                1
        );
        frameBuffers.push_back(resources.device.createFramebuffer(info));
    }
}

vk::CommandBuffer ImguiUi::updateCommandBuffer(uint32_t index, UiBindings &bindings) {
    if (commandBuffers.empty()) {
        initCommandBuffers();
    }

    auto cmd = commandBuffers[index];

    cmd.reset();


    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    drawUi(bindings);

    ImGui::Render();

    vk::CommandBufferBeginInfo cmdBeginInfo = {};

    cmd.begin(cmdBeginInfo);

    vk::ClearValue color(std::array<float, 4>{0, 0, 0, 0});

    vk::Rect2D area({0, 0}, resources.extent);

    vk::RenderPassBeginInfo passBeginInfo(renderPass, frameBuffers[index], area, color);
    cmd.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    cmd.endRenderPass();

    cmd.end();

    return cmd;
}

void ImguiUi::drawUi(UiBindings &bindings) {
    if (bindings.renderParameters.showDemoWindow)
        ImGui::ShowDemoWindow();

    ImGui::Begin("Settings");

    if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &render = bindings.renderParameters;
        ImGui::Checkbox("Show demo window", &render.showDemoWindow);
        ImGui::Checkbox("DebugPhysics", &render.debugImagePhysics);
        ImGui::Checkbox("DebugSort", &render.debugImageSort);
        ImGui::Checkbox("DebugRender", &render.debugImageRenderer);
    }

    if (ImGui::CollapsingHeader("Simulation Parameters")) {
        bindings.updateFlags.resetSimulation |= ImGui::Button("Restart Simulation");

        auto &simulation = bindings.simulationParameters;
        ImGui::DragInt("Num Particles", reinterpret_cast<int*>(&simulation.numParticles), 16, 16, 1024 * 1024 * 1024);
    }

    ImGui::End();
}

void ImguiUi::destroyCommandBuffers() {
    if (commandPool) {
        commandBuffers.clear();
        resources.device.destroyCommandPool(commandPool);
        commandPool = nullptr;
    }
    if (!frameBuffers.empty()) {
        for (auto framebuffer: frameBuffers) resources.device.destroyFramebuffer(framebuffer);
        frameBuffers.clear();
    }
}

ImguiUi::~ImguiUi() {
    destroyCommandBuffers();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    resources.device.destroyRenderPass(renderPass);
    resources.device.destroyDescriptorPool(descriptorPool);
}