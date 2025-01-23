#include "simulation.h"
#include <map>
#include <set>

#include "debug.h"
#include "debug_image.h"
#include "particle_physics.h"
#include "particle_renderer.h"
#include "render.h"
#include "spatial_lookup.h"

Simulation::Simulation(const RenderParameters &_renderParameters,
                       const SimulationParameters &_simulationParameters,
                       std::shared_ptr<Camera> camera)
    : renderParameters(_renderParameters), simulationParameters(_simulationParameters), simulationState(std::make_unique<SimulationState>(simulationParameters, std::move(camera))) {

    particlePhysics = std::make_unique<ParticleSimulation>(simulationParameters);
    spatialLookup = std::make_unique<SpatialLookup>(simulationParameters);
    imguiUi = std::make_unique<ImguiUi>();
    particleRenderer = std::make_unique<ParticleRenderer>();

    vk::CommandBufferAllocateInfo cmdAllocateInfo(resources.transferCommandPool, vk::CommandBufferLevel::ePrimary, 3);
    auto allocated = resources.device.allocateCommandBuffers(cmdAllocateInfo);

    cmdCopy = allocated[0];
    cmdReset = allocated[1];
    cmdEmpty = allocated[2];

    reset();
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
    float spatialRadius = simulationState->spatialRadius;

    const size_t cmd_count = 6;

    if (nullptr != timelineSemaphore) {
        vk::SemaphoreWaitInfo waitInfo({}, timelineSemaphore, cmd_count);
        vk::detail::resultCheck(resources.device.waitSemaphores(waitInfo, -1), "Failed wait");
    }

    auto results = resources.device.getQueryPoolResults<uint64_t>(resources.queryPool, 0, Query::COUNT, Query::COUNT * sizeof(uint64_t), sizeof(uint64_t)).value;
    timestamps.clear();
    for (int i = 0; i < Query::COUNT; ++i) {
        timestamps.emplace(static_cast<Query>(i), static_cast<double>(results[i]) * resources.timestampPeriod);
    }

    processUpdateFlags(lastUpdate);

    timelineSemaphore = initSemaphore();

    UiBindings uiBindings {imageIndex, simulationParameters, renderParameters, simulationState.get(), QueryTimes(timestamps)};

    auto imguiCommandBuffer = imguiUi->updateCommandBuffer(imageIndex, uiBindings);
    lastUpdate = uiBindings.updateFlags;

    double currentTime = glfwGetTime();
    double delta = (simulationState->paused ? 0 : currentTime - prevTime) * 1000;
    prevTime = currentTime;

    if (simulationState->step) {
        simulationState->step = false;
        delta = simulationState->time.tickRate;
    }

    bool doTick = simulationState->time.advance(delta);

    std::array<std::tuple<vk::Queue, vk::CommandBuffer>, cmd_count> buffers;
    buffers[0] = {resources.transferQueue, cmdReset};
    buffers[1] = {resources.computeQueue, doTick ? particlePhysics->run(*simulationState) : nullptr};
    buffers[2] = {resources.computeQueue, spatialLookup->run(*simulationState)};
    buffers[3] = {resources.graphicsQueue, particleRenderer->run(*simulationState, renderParameters)};
    buffers[4] = {resources.graphicsQueue, copy(imageIndex)};
    buffers[5] = {resources.graphicsQueue, imguiCommandBuffer};

    for (uint64_t wait = 0, signal = 1; wait < buffers.size(); ++wait, ++signal) {
        auto queue = std::get<0>(buffers[wait]);
        auto cmd = std::get<1>(buffers[wait]);
        queue = nullptr == cmd ? resources.transferQueue : queue;
        cmd = nullptr == cmd ? cmdEmpty : cmd;

        vk::TimelineSemaphoreSubmitInfo timeline(
                wait,
                signal);

        std::array<vk::PipelineStageFlags, 1> flags {vk::PipelineStageFlagBits::eAllCommands};
        vk::SubmitInfo submit(
                timelineSemaphore,
                flags,
                cmd,
                timelineSemaphore,
                &timeline);

        if (wait == 0) {// first submit has no dependencies
            submit.waitSemaphoreCount = 0;
            timeline.waitSemaphoreValueCount = 0;
        }

        if (cmd == cmdCopy) {// copy needs to wait for the swapchain image
            auto waitSemaphores = std::array<vk::Semaphore, 2> {timelineSemaphore, waitImageAvailable};
            auto waitSemaphoreValues = std::array<uint64_t, 2> {wait, 0};
            auto waitStageFlags = std::array<vk::PipelineStageFlags, 2> {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer};

            submit.setWaitSemaphores(waitSemaphores);
            timeline.setWaitSemaphoreValues(waitSemaphoreValues);
            submit.setWaitDstStageMask(waitStageFlags);
            queue.submit(submit);
            continue;
        }

        if (wait == buffers.size() - 1) {// last submit signals submit-finished
            auto signalSemaphores = std::array<vk::Semaphore, 2> {timelineSemaphore, signalRenderFinished};
            auto signalSemaphoreValues = std::array<uint64_t, 2> {signal, 0};

            submit.setSignalSemaphores(signalSemaphores);
            timeline.setSignalSemaphoreValues(signalSemaphoreValues);

            queue.submit(submit, signalSubmitFinished);
            continue;
        }

        queue.submit(submit);
    }

    if (!lastUpdate.runChecks) {
        return;
    }

    // FOR DEBUGGING

    resources.device.waitIdle();

    std::vector<float> particles(simulationParameters.numParticles * (simulationParameters.type == SceneType::SPH_BOX_2D ? 2 : 4));
    fillHostWithStagingBuffer(simulationState->particleCoordinateBuffer, particles);

    uint32_t lookupSize = nextPowerOfTwo(simulationParameters.numParticles);
    std::vector<SpatialLookupEntry> spatial_lookup(lookupSize);
    fillHostWithStagingBuffer(simulationState->spatialLookup, spatial_lookup);
    std::vector<uint32_t> spatial_lookup_keys(lookupSize);
    for (int i = 0; i < spatial_lookup.size(); ++i) spatial_lookup_keys[i] = spatial_lookup[i].cellKey;

    std::vector<uint32_t> spatial_indices(lookupSize);
    fillHostWithStagingBuffer(simulationState->spatialIndices, spatial_indices);

    std::vector<SpatialLookupEntry> spatial_lookup_sorted(spatial_lookup.begin(), spatial_lookup.end());
    std::sort(spatial_lookup_sorted.begin(), spatial_lookup_sorted.end(),
              [](SpatialLookupEntry left, SpatialLookupEntry right) -> bool { return left.cellKey < right.cellKey; });

    std::set<uint32_t> keys;
    std::vector<SpatialHashResult> hashes;
    for (uint32_t i = 0; i < lookupSize; i++) {
        SpatialLookupEntry lookup = spatial_lookup[i];

        keys.insert(lookup.cellKey);

        if (lookup.particleIndex == -1) continue;

        glm::vec3 position;
        switch (simulationParameters.type) {
            case SceneType::SPH_BOX_2D:
                position = glm::vec3(particles[lookup.particleIndex * 2], particles[lookup.particleIndex * 2 + 1], 0);
                break;
            case SceneType::SPH_BOX_3D:
                position = glm::vec3(particles[lookup.particleIndex * 4], particles[lookup.particleIndex * 4 + 1], particles[lookup.particleIndex * 4 + 2]);
                break;
        }

        glm::ivec3 cell = cellCoord(position, spatialRadius);
        uint32_t testKey = cellKey(cellHash(cell), simulationParameters.numParticles);

        SpatialHashResult result {
                lookup.cellKey,
                testKey,
                position.x,
                position.y,
                position.z,
                cell.x,
                cell.y,
                cell.z,
        };

        keys.emplace(lookup.cellKey);
        hashes.emplace_back(result);

        if (result.lookupKey != result.testKey) {
            throw std::runtime_error("key differs");
        }

        if (spatial_lookup[i].cellKey != spatial_lookup_sorted[i].cellKey) {
            throw std::runtime_error("spatial lookup not sorted");
        }
    }

    std::map<uint32_t, std::vector<SpatialHashResult>> groupedResults;

    for (const auto &hash: hashes) {
        uint32_t lookupKey = hash.lookupKey;

        auto &group = groupedResults[lookupKey];
        bool isDistinct = true;

        for (const auto &element: group) {
            if (element.cellX == hash.cellX && element.cellY == hash.cellY && element.cellZ == hash.cellZ) {
                isDistinct = false;
                break;
            }
        }

        if (isDistinct) {
            group.push_back(hash);
        }
    }

    std::map<uint32_t, std::vector<SpatialHashResult>> collisions;

    for (const auto &[lookupKey, group]: groupedResults) {
        if (group.size() >= 2) {
            collisions[lookupKey] = group;
        }
    }

    uint32_t collisionCellCount = 0;
    for (const auto &item: collisions) collisionCellCount += item.second.size();

    std::cout << "Hash-Collisions "
              << "Key-Count: " << keys.size() << " "
              << "Collision-Count: " << collisions.size() << " "
              << "Cell-Count: " << collisionCellCount << " "
              << std::endl;
    int a = 0;
}

vk::CommandBuffer Simulation::copy(uint32_t imageIndex) {

    vk::Image srcImage = particleRenderer->getImage();
    vk::ImageLayout srcImageLayout = vk::ImageLayout::eColorAttachmentOptimal;

    switch (renderParameters.selectedImage) {
        case SelectedImage::DEBUG_PHYSICS:
            srcImage = simulationState->debugImageSort->image;
            srcImageLayout = vk::ImageLayout::eGeneral;
            break;
        case SelectedImage::DEBUG_SORT:
            srcImage = simulationState->debugImageRenderer->image;
            srcImageLayout = vk::ImageLayout::eGeneral;
            break;
        case SelectedImage::DEBUG_RENDERER:
            srcImage = simulationState->debugImagePhysics->image;
            srcImageLayout = vk::ImageLayout::eGeneral;
            break;
        default:
            break;
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
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        cmdCopy.pipelineBarrier(
                srcStageMask,
                dstStageMask,
                {},
                nullptr,
                nullptr,
                barrier);
    };

    cmdCopy.reset();

    cmdCopy.begin(vk::CommandBufferBeginInfo());
    writeTimestamp(cmdCopy, CopyBegin);

    // transition swapchain image
    barrier(
            resources.swapchainImages[imageIndex],
            {},
            vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer);

    // transition source image to transfer source
    barrier(
            srcImage,
            {},
            vk::AccessFlagBits::eTransferRead,
            srcImageLayout,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer);

    vk::ImageCopy copy(
            {{vk::ImageAspectFlagBits::eColor}, 0, 0, 1},
            {},
            {{vk::ImageAspectFlagBits::eColor}, 0, 0, 1},
            {},
            {resources.extent.width, resources.extent.height, 1});

    cmdCopy.copyImage(
            srcImage,
            vk::ImageLayout::eTransferSrcOptimal,
            resources.swapchainImages[imageIndex],
            vk::ImageLayout::eTransferDstOptimal,
            copy);

    // transition swapchain image to present
    barrier(
            resources.swapchainImages[imageIndex],
            vk::AccessFlagBits::eTransferWrite,
            {},
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe);

    // transition source image back to original layout
    barrier(
            srcImage,
            vk::AccessFlagBits::eTransferRead,
            {},
            vk::ImageLayout::eTransferSrcOptimal,
            srcImageLayout,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe);

    writeTimestamp(cmdCopy, CopyEnd);
    cmdCopy.end();

    return cmdCopy;
}

Simulation::~Simulation() {
    resources.device.destroySemaphore(timelineSemaphore);
}

void Simulation::processUpdateFlags(const UpdateFlags &updateFlags) {
    if (updateFlags.resetSimulation) {
        auto newState = std::make_unique<SimulationState>(simulationParameters, simulationState->camera);
        simulationState = std::move(newState);
        reset();
    } else if (updateFlags.loadSceneFromFile) {
        auto [r, s] = SceneParameters::loadParametersFromFile(imguiUi->getSelectedSceneFile());
        renderParameters = r;
        simulationParameters = s;

        auto newState = std::make_unique<SimulationState>(simulationParameters, simulationState->camera);
        simulationState = std::move(newState);
        reset();
    }

    if (updateFlags.togglePause)
        simulationState->paused = !simulationState->paused;

    // advance sets paused to true for only one step (see run()), needs to be reset here
    if (updateFlags.stepSimulation) {
        simulationState->paused = true;
        simulationState->step = true;
    }

    if (updateFlags.printRenderSettings) {
        std::cout << "-------------------- Render Settings --------------------\n";
        std::cout << renderParameters.printToYaml() << std::endl;
        std::cout << "---------------------------------------------------------\n";
    }

    // moved here to update every frame, since MVP matrix is a push-constant
    updateCommandBuffers();
}

void Simulation::updateCommandBuffers() {
    particleRenderer->updateCmd(*simulationState, renderParameters);
}

void Simulation::reset() {
    std::cout << "Simulation reset" << std::endl;

    {
        auto cmd = beginSingleTimeCommands(resources.device, resources.transferCommandPool);
        cmd.resetQueryPool(resources.queryPool, 0, Query::COUNT);
        endSingleTimeCommands(resources.device, resources.transferQueue, resources.transferCommandPool, cmd);
    }
    
    cmdReset.reset();

    cmdReset.begin(vk::CommandBufferBeginInfo());

    writeTimestamp(cmdReset, ResetBegin);
    simulationState->debugImagePhysics->clear(cmdReset, {1, 0, 0, 1});
    simulationState->debugImageSort->clear(cmdReset, {0, 1, 0, 1});
    simulationState->debugImageRenderer->clear(cmdReset, {0, 0, 1, 1});
    writeTimestamp(cmdReset, ResetEnd);

    cmdReset.end();

    cmdEmpty.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
    cmdEmpty.end();

    // the spatial-lookup needs to always run at least once before any run
    spatialLookup->updateCmd(*simulationState);
    particlePhysics->updateCmd(*simulationState);

    {
        vk::SubmitInfo submit({}, {}, cmdReset);
        resources.transferQueue.submit(submit);
        resources.device.waitIdle();
    }

    auto cmd = spatialLookup->run(*simulationState);
    if (nullptr != cmd) {
        vk::SubmitInfo submit({}, {}, cmd);
        resources.computeQueue.submit(submit);
        resources.device.waitIdle();
    }

    prevTime = glfwGetTime();

    std::cout << "Simulation reset done" << std::endl;
}