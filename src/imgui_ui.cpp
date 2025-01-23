#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_ui.h"
#include "simulation_parameters.h"
#include "simulation_state.h"

ImguiUi::ImguiUi() {
    const vk::DescriptorPoolSize pool_sizes[] =
            {
                    {vk::DescriptorType::eSampler, 10},
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
            vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription ui_subpass(
            {},
            vk::PipelineBindPoint::eGraphics,
            {},
            color_attachment,
            {},
            nullptr,
            {});

    vk::SubpassDependency externalDependency(
            VK_SUBPASS_EXTERNAL,
            0,
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::PipelineStageFlagBits::eColorAttachmentOutput},
            {vk::AccessFlagBits::eColorAttachmentWrite},
            {vk::AccessFlagBits::eColorAttachmentWrite});

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
            0};

    ImGui_ImplVulkan_Init(&init_info, renderPass);
    ImGui::StyleColorsDark();

    ImGui_ImplVulkan_CreateFontsTexture();

    for (auto entry: std::filesystem::directory_iterator {"../scenes/"}) {
        if (!entry.is_regular_file())
            continue;
        auto path = entry.path();
        auto extension = path.extension().string();
        if (extension == ".yaml" || extension == ".yml") {
            auto i = sceneFiles.size();
            sceneFiles.emplace_back(path.filename().string());

            if (path.stem().string() == "default")
                currentSceneFile = i;
        }
    }

    for (size_t i = 0; i < sceneFiles.size(); i++) {
        sceneFilesCStr.emplace_back(sceneFiles[i].c_str());
    }
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
                1);
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

    vk::ClearValue color(std::array<float, 4> {0, 0, 0, 0});

    vk::Rect2D area({0, 0}, resources.extent);

    vk::RenderPassBeginInfo passBeginInfo(renderPass, frameBuffers[index], area, color);
    cmd.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    cmd.endRenderPass();

    cmd.end();

    return cmd;
}

template<typename T>
bool EnumCombo(const char *name, T *currentValue, const Mappings<T> &mappings) {
    // static should be per type
    static const auto comboValueArray = imguiComboArray(mappings);
    return ImGui::Combo(name, reinterpret_cast<int *>(currentValue), comboValueArray.data(), comboValueArray.size());
}

void ImguiUi::drawUi(UiBindings &bindings) {
    auto &updateFlags = bindings.updateFlags;

    if (bindings.renderParameters.showDemoWindow)
        ImGui::ShowDemoWindow();

    ImGui::Begin("Settings");
    ImGui::PushItemWidth(120);

    bool paused = !bindings.simulationState || bindings.simulationState->paused;
    updateFlags.togglePause = ImGui::Button(paused ? "Resume" : "Pause ");
    ImGui::SameLine();
    updateFlags.stepSimulation = ImGui::Button("Step");
    ImGui::SameLine();
    updateFlags.resetSimulation = ImGui::Button("Reset");
    ImGui::SameLine();
    updateFlags.runChecks = ImGui::Button("Check");

    ImGui::Text("Ticks: %d", bindings.simulationState->time.ticks);
    ImGui::DragInt("Tick rate:", &bindings.simulationState->time.tickRate, 5, 5, 5000);

    if (ImGui::CollapsingHeader("Scene Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Combo("##sceneFile", &currentSceneFile, sceneFilesCStr.data(), sceneFilesCStr.size());
        ImGui::SameLine();
        updateFlags.loadSceneFromFile = ImGui::Button("Load File");
    }

    if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &render = bindings.renderParameters;
        EnumCombo("Selected Image", &render.selectedImage, selectedImageMappings);

        EnumCombo("Background Field", &render.backgroundField, renderBackgroundFieldMappings);
        EnumCombo("Particle Color", &render.particleColor, renderParticleColorMappings);

        ImGui::DragFloat("Particle Radius", &render.particleRadius, 0.5, 1.0, 64.0, "%.1f");
        ImGui::DragFloat("Spatial Radius", &bindings.simulationState->spatialRadius, 0.01, 0.01, 1.0, "%.2f");

        ImGui::Separator();
        updateFlags.printRenderSettings = ImGui::Button("Print Render Settings");
    }

    if (ImGui::CollapsingHeader("Simulation Parameters")) {
        updateFlags.resetSimulation |= ImGui::Button("Restart Simulation");

        auto &simulation = bindings.simulationParameters;
        EnumCombo("Scene Type", &simulation.type, sceneTypeMappings);
        ImGui::DragInt("Num Particles", reinterpret_cast<int *>(&simulation.numParticles), 16, 16, 1024 * 1024 * 1024);
        ImGui::DragFloat("Gravity", &simulation.gravity, 0.1f);
        ImGui::DragFloat("Delta Time", &simulation.deltaTime, 0.001f);
        ImGui::DragFloat("Collision Damping", &simulation.collisionDampingFactor, 0.01f);
        ImGui::DragFloat("Target Density", &simulation.targetDensity, 0.01f);
        ImGui::DragFloat("Pressure Multiplier", &simulation.pressureMultiplier, 0.01f);
    }

#ifdef _DEBUG
    if (ImGui::CollapsingHeader("Debug")) {
        ImGui::Checkbox("ImGui demo window", &bindings.renderParameters.showDemoWindow);
    }
#endif
    ImGui::End();
}

void ImguiUi::destroyCommandBuffers() {
    if (commandPool) {
        commandBuffers.clear();
        resources.device.destroyCommandPool(commandPool);
        commandPool = nullptr;
    }
    if (!frameBuffers.empty()) {
        for (auto framebuffer: frameBuffers)
            resources.device.destroyFramebuffer(framebuffer);
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

std::string ImguiUi::getSelectedSceneFile() const {
    return "../scenes/" + sceneFiles[currentSceneFile];
}
