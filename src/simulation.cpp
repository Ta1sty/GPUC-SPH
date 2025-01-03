#include "simulation.h"

#include "particle_renderer.h"
#include "particle_physics.h"
#include "hashgrid.h"
#include "render.h"
#include "debug_image.h"

std::vector<float> initUniform(SceneType sceneType, uint32_t numParticles, std::mt19937 &random) {
    std::vector<float> values;

    switch (sceneType) {
        case SceneType::SPH_BOX_2D:
            values.resize(2 * numParticles);
            std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
            for (auto &v : values) {
                v = distribution(random);
            }
    }

    return values;
}

std::vector<float> initPoissonDisk(SceneType sceneType, uint32_t numParticles, std::mt19937 &random) {
    throw std::exception("initPoissonDisk() is not implemented");

    std::vector<float> values;

    switch (sceneType) {
        case SceneType::SPH_BOX_2D:
            break;
    }
}

SimulationState::SimulationState(const SimulationParameters &parameters) {
    camera = std::make_unique<Camera>();
    debugImagePhysics = std::make_unique<DebugImage>("debug-image-particle");
    debugImageSort = std::make_unique<DebugImage>("debug-image-sort");
    debugImageRenderer = std::make_unique<DebugImage>("debug-image-render");

    switch (parameters.type) {
        case SceneType::SPH_BOX_2D:
            coordinateBufferSize = sizeof(Particle) * parameters.numParticles;
            break;
    }

    random.seed(parameters.randomSeed);

    std::vector<float> values;

    switch (parameters.initializationFunction) {
        case InitializationFunction::UNIFORM:
            values = initUniform(parameters.type, parameters.numParticles, random);
            break;
        case InitializationFunction::POISSON_DISK:
            values = initPoissonDisk(parameters.type, parameters.numParticles, random);
            break;
    }

    createBuffer(
            resources.pDevice,
            resources.device,
            coordinateBufferSize,
            {vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc},
            {vk::MemoryPropertyFlagBits::eDeviceLocal},
            "buffer-particles",
            particleCoordinateBuffer,
            particleMemory
    );

    createBuffer(
            resources.pDevice,
            resources.device,
            parameters.numParticles * sizeof(HashGridCell),
            {vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc},
            {vk::MemoryPropertyFlagBits::eDeviceLocal},
            "buffer-hash-grid",
            particleCoordinateBuffer,
            particleMemory
    );

    Buffer particleBuffer { particleCoordinateBuffer, particleMemory }; // TODO this is kind of dumb
    fillDeviceWithStagingBuffer(resources.pDevice, resources.device, resources.transferCommandPool, resources.transferQueue,
                                particleBuffer, values);
}

Simulation::Simulation(const SimulationParameters &parameters) : simulationParameters(parameters),
                                                                 simulationState(std::make_unique<SimulationState>(parameters)) {
    particlePhysics = std::make_unique<ParticleSimulation>();
    hashGrid = std::make_unique<HashGrid>();
    imguiUi = std::make_unique<ImguiUi>();
    particleRenderer = std::make_unique<ParticleRenderer>(simulationParameters);

    vk::FenceCreateInfo timelineFenceInfo(vk::FenceCreateFlagBits::eSignaled);
    timelineFence = resources.device.createFence(timelineFenceInfo);

    vk::CommandBufferAllocateInfo cmdAllocateInfo(resources.transferCommandPool, vk::CommandBufferLevel::ePrimary, 3);
    auto allocated = resources.device.allocateCommandBuffers(cmdAllocateInfo);

    cmdCopy = allocated[0];

    cmdReset = allocated[1];
    cmdReset.begin(vk::CommandBufferBeginInfo());
    simulationState->debugImagePhysics->clear(cmdReset, { 1, 0, 0, 1 });
    simulationState->debugImageSort->clear(cmdReset, { 0, 1, 0, 1 });
    simulationState->debugImageRenderer->clear(cmdReset, { 0, 0, 1, 1 });
    cmdReset.end();

    cmdEmpty = allocated[2];
    cmdEmpty.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
    cmdEmpty.end();
}

SimulationState::~SimulationState() {
    auto Bclean = [&](Buffer& b) {
        resources.device.destroyBuffer(b.buf);
        resources.device.freeMemory(b.mem);
    };

    resources.device.destroyBuffer(particleCoordinateBuffer);
    resources.device.freeMemory(particleMemory);
}

vk::Semaphore Simulation::initSemaphore() {
    if (nullptr != timelineSemaphore) {
        resources.device.destroySemaphore(timelineSemaphore);
        timelineSemaphore = nullptr;
    }

    vk::SemaphoreTypeCreateInfo timelineSemaphoreType(vk::SemaphoreType::eTimeline, 0);
    vk::SemaphoreCreateInfo timelineSemaphoreInfo({}, &timelineSemaphoreType);

    return resources.device.createSemaphore(timelineSemaphoreInfo);
}

void Simulation::run(uint32_t imageIndex, vk::Semaphore waitImageAvailable, vk::Semaphore signalRenderFinished, vk::Fence signalSubmitFinished) {
    const size_t count = 6;

    if (nullptr != timelineSemaphore) {
        vk::SemaphoreWaitInfo waitInfo({}, timelineSemaphore, count);
        vk::detail::resultCheck(resources.device.waitSemaphores(waitInfo, -1), "Failed wait");
    }

    timelineSemaphore = initSemaphore();

    // TODO @markus stimmt imageIndex hier?
    UiBindings uiBindings { imageIndex, simulationParameters, renderParameters };
    std::array<std::tuple<vk::Queue,vk::CommandBuffer>,count> buffers;

    // TODO temporarily changed to transfer queue because
    //  vulkan doesn't like stuff being allocated from the transfer
    //  queue being run elsewhere
    buffers[0] = { resources.transferQueue, cmdReset };
    buffers[1] = { resources.transferQueue,  particlePhysics->run() };
    buffers[2] = { resources.transferQueue,  hashGrid->run() };
    buffers[3] = { resources.transferQueue, particleRenderer->run(*simulationState)};
    buffers[4] = { resources.graphicsQueue, copy(imageIndex) };
    buffers[5] = { resources.graphicsQueue, imguiUi->updateCommandBuffer(imageIndex, uiBindings) };

    auto flags = uiBindings.updateFlags;
    if (flags.resetSimulation) {
        // TODO needs to be done after fence (destroys buffers in use) or keep old instance as weak reference somewhere
        // simulationState = std::make_unique<SimulationState>(simulationParameters);
    }

    for (uint64_t wait = 0,signal = 1; wait < buffers.size(); ++wait,++signal) {
        auto queue = std::get<0>(buffers[wait]);
        auto cmd = std::get<1>(buffers[wait]);
        cmd = nullptr == cmd ? cmdEmpty : cmd;

        vk::TimelineSemaphoreSubmitInfo timeline(
                wait,
                signal
        );

        std::array<vk::PipelineStageFlags,1> flags{ vk::PipelineStageFlagBits::eAllCommands };
        vk::SubmitInfo submit(
                timelineSemaphore,
                flags,
                cmd,
                timelineSemaphore,
                &timeline
        );

        if (wait == 0) { // first submit has no dependencies
            submit.waitSemaphoreCount = 0;
            timeline.waitSemaphoreValueCount = 0;
        }

        if (cmd == cmdCopy) { // copy needs to wait for the swapchain image
            auto waitSemaphores = std::array<vk::Semaphore,2> {timelineSemaphore, waitImageAvailable};
            auto waitSemaphoreValues = std::array<uint64_t ,2> {wait, 0};
            auto waitStageFlags = std::array<vk::PipelineStageFlags, 2> {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer };

            submit.setWaitSemaphores(waitSemaphores);
            timeline.setWaitSemaphoreValues(waitSemaphoreValues);
            submit.setWaitDstStageMask(waitStageFlags);
        }

        if (wait == buffers.size() - 1){ // last submit signals render-finished and the submit-finished
            auto signalSemaphores = std::array<vk::Semaphore,2> {timelineSemaphore, signalRenderFinished};
            auto signalSemaphoreValues = std::array<uint64_t ,2> {signal, 0};

            submit.setSignalSemaphores(signalSemaphores);
            timeline.setSignalSemaphoreValues(signalSemaphoreValues);

            queue.submit(submit, signalSubmitFinished);
            continue;
        }

        queue.submit(submit);
    }
}

vk::CommandBuffer Simulation::copy(uint32_t imageIndex) {

    vk::Image srcImage = particleRenderer->getImage();
    vk::ImageLayout srcImageLayout = vk::ImageLayout::eColorAttachmentOptimal;

    if (renderParameters.debugImagePhysics) {
        srcImage = simulationState->debugImagePhysics->image;
        srcImageLayout = vk::ImageLayout::eGeneral;
    }
    if (renderParameters.debugImageSort) {
        srcImage = simulationState->debugImageSort->image;
        srcImageLayout = vk::ImageLayout::eGeneral;
    }
    if (renderParameters.debugImageRenderer) {
        srcImage = simulationState->debugImageRenderer->image;
        srcImageLayout = vk::ImageLayout::eGeneral;
    }

    auto barrier = [&](vk::Image image,
                                        vk::AccessFlags srcAccessMask,
                                        vk::AccessFlags dstAccessMask,
                                        vk::ImageLayout oldLayout,
                                        vk::ImageLayout newLayout,
                                        vk::PipelineStageFlags srcStageMask,
                                        vk::PipelineStageFlags dstStageMask) {
        vk::ImageMemoryBarrier barrier(
                srcAccessMask,
                dstAccessMask,
                oldLayout,
                newLayout,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                image,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        );
        cmdCopy.pipelineBarrier(
                srcStageMask,
                dstStageMask,
                {},
                nullptr,
                nullptr,
                barrier
        );
    };

    cmdCopy.reset();

    cmdCopy.begin(vk::CommandBufferBeginInfo());

    // transition swapchain image
    barrier(
            resources.swapchainImages[imageIndex],
            {},
            vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer
    );

    // transition source image to transfer source
    barrier(
            srcImage,
            {},
            vk::AccessFlagBits::eTransferRead,
            srcImageLayout,
            vk::ImageLayout::eTransferSrcOptimal,
             vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer
    );

    vk::ImageCopy copy(
            { {vk::ImageAspectFlagBits::eColor}, 0,0,1},
            {},
            { {vk::ImageAspectFlagBits::eColor}, 0,0,1},
            {},
            {resources.extent.width, resources.extent.height, 1}
    );

    cmdCopy.copyImage(
            srcImage,
            vk::ImageLayout::eTransferSrcOptimal,
            resources.swapchainImages[imageIndex],
            vk::ImageLayout::eTransferDstOptimal,
            copy
    );

    // transition swapchain image to present
    barrier(
            resources.swapchainImages[imageIndex],
            vk::AccessFlagBits::eTransferWrite,
            {},
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe
    );

    // transition source image back to original layout
    barrier(
            srcImage,
            vk::AccessFlagBits::eTransferRead,
            {},
            vk::ImageLayout::eTransferSrcOptimal,
            srcImageLayout,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe
    );

    cmdCopy.end();

    return cmdCopy;
}

Simulation::~Simulation() {
    // TODO clean up your vulkan objects!
}

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

