#include <iostream>
#include <fstream>
#include <cstring>
#include <functional>
#include <optional>

#define VK_ENABLE_BETA_EXTENSIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "initialization.h"
#include "utils.h"

#include <optional>

// here you create the instance and physical / logical device and maybe compute/transfer queues
// also check if device is suitable etc


AppResources dontUse = AppResources();

AppResources &resources = dontUse;

struct DeviceSelectionCache {
    uint32_t vendorID;
    uint32_t deviceID;
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
#ifndef NDEBUG
    "VK_LAYER_KHRONOS_validation"
#endif
};
const std::vector<const char*> staticInstanceExtensions = {
#ifndef NDEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

const std::vector<const char*> staticExtensionNames = {
#ifndef NDEBUG

#endif
};

void AppResources::destroy() {
    this->device.destroyQueryPool(this->queryPool);
    //this->device.freeCommandBuffers(this->computeCommandPool, 1U, &this->computeCommandBuffer);
    //this->device.freeCommandBuffers(this->transferCommandPool, 1U, &this->transferCommandBuffer);
    this->device.destroyCommandPool(this->computeCommandPool);
    //this->device.destroyCommandPool(this->transferCommandPool);
    this->device.destroyCommandPool(this->graphicsCommandPool);

    for (auto imageView : this->swapchainImageViews) {
        this->device.destroyImageView(imageView);
    }
    this->swapchainImageViews.resize(0);

    if (this->swapchain)
        this->device.destroySwapchainKHR(this->swapchain);
    this->swapchainImages.resize(0);

    this->device.destroy();

    if (this->surface)
        this->instance.destroySurfaceKHR(this->surface);

#ifndef NDEBUG
    this->instance.destroyDebugUtilsMessengerEXT(this->dbgUtilsMgr);
#endif
    this->instance.destroy();

    glfwDestroyWindow(this->window);
}

void initApp(AppResources& app, bool withWindow, const std::string& name, int width, int height) {
    app.window = nullptr;
    if (withWindow) {
        if (!glfwInit())
            throw std::runtime_error("GLFW initialization failed!");
        if (!glfwVulkanSupported())
            throw std::runtime_error(
                "GLFW reports to have no Vulkan support! Maybe it couldn't find the Vulkan loader!");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        app.window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    }

    createInstance(app.instance, app.dbgUtilsMgr, "Assignment1, Task 1", "Idkwhattowrite", withWindow);

    app.surface = VK_NULL_HANDLE;
    if (withWindow) {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(app.instance, app.window, NULL, &surface))
            throw std::runtime_error("Surface creation failed!");
        app.surface = surface;
    }

    selectPhysicalDevice(app.instance, app.pDevice);
    // printDeviceCapabilities(app.pDevice);
    std::tie(app.gQ, app.cQ, app.tQ) = getGCTQueues(app.pDevice);
    app.tQ = -1;
    createLogicalDevice(app.instance, app.pDevice, app.device, withWindow);

    app.device.getQueue(app.gQ, 0U, &app.graphicsQueue);
    createCommandPool(app.device, app.graphicsCommandPool, app.gQ);


    if (app.cQ != -1) {
        app.device.getQueue(app.cQ, 0U, &app.computeQueue);
        createCommandPool(app.device, app.computeCommandPool, app.cQ);
    }
    else {
        app.computeQueue = app.graphicsQueue;
        app.computeCommandPool = app.graphicsCommandPool;
    }

    if (app.tQ != -1) {
        app.device.getQueue(app.tQ, 0U, &app.transferQueue);
        createCommandPool(app.device, app.transferCommandPool, app.tQ);
    }
    else {
        app.transferQueue = app.graphicsQueue;
        app.transferCommandPool = app.graphicsCommandPool;
    }

    createTimestampQueryPool(app.device, app.queryPool, 2);

    app.swapchain = VK_NULL_HANDLE;
    if (withWindow)
        createSwapchain(app);

    resources = app;
}

/*
    This is the function in which errors will go through to be displayed.
*/
VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                            VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                            void* /*pUserData*/) {
    if (enableValidationLayers) {
        if (pCallbackData->messageIdNumber == 648835635) {
            // UNASSIGNED-khronos-Validation-debug-build-warning-message
            return VK_FALSE;
        }
        if (pCallbackData->messageIdNumber == 767975156) {
            // UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
            return VK_FALSE;
        }
    }

    std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
        << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << ":\n";
    std::cerr << "\t"
        << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    std::cerr << "\t"
        << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    std::cerr << "\t"
        << "message         = <" << pCallbackData->pMessage << ">\n";
    if (0 < pCallbackData->queueLabelCount) {
        std::cerr << "\t"
            << "Queue Labels:\n";
        for (uint8_t i = 0; i < pCallbackData->queueLabelCount; i++) {
            std::cerr << "\t\t"
                << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount) {
        std::cerr << "\t"
            << "CommandBuffer Labels:\n";
        for (uint8_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
            std::cerr << "\t\t"
                << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount) {
        std::cerr << "\t"
            << "Objects:\n";
        for (uint8_t i = 0; i < pCallbackData->objectCount; i++) {
            std::cerr << "\t\t"
                << "Object " << i << "\n";
            std::cerr << "\t\t\t"
                << "objectType   = "
                << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType)) << "\n";
            std::cerr << "\t\t\t"
                << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
            if (pCallbackData->pObjects[i].pObjectName) {
                std::cerr << "\t\t\t"
                    << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }
    return VK_TRUE;
}

/*
    This function fills the structure with flags indicating
    which error messages should go through
*/
vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT() {
    using SEVERITY = vk::DebugUtilsMessageSeverityFlagBitsEXT; // for readability
    using MESSAGE = vk::DebugUtilsMessageTypeFlagBitsEXT;
    return {
        {},
        SEVERITY::eWarning | SEVERITY::eError,
        MESSAGE::eGeneral | MESSAGE::ePerformance | MESSAGE::eValidation,
        &debugUtilsMessengerCallback
    };
}

/*
    The dynamic loader allows us to access many extensions
    Required before creating instance for loading the extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME
*/
void initDynamicLoader() {
    static vk::DynamicLoader dl;
    static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>(
        "vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
}

void createInstance(vk::Instance& instance, vk::DebugUtilsMessengerEXT& debugUtilsMessenger,
                    std::string appName, std::string engineName, bool withWindow) {
    initDynamicLoader();
    vk::ApplicationInfo applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, VK_API_VERSION_1_2);

    std::vector<const char*> instanceExtensions(staticInstanceExtensions);
    if (withWindow) {
        // GLFW gives us the platform specific extensions for surface handling
        uint32_t count;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        instanceExtensions.insert(instanceExtensions.end(), extensions, extensions + count);
    }

    // ### initialize the InstanceCreateInfo ###
    vk::InstanceCreateInfo instanceCreateInfo( //flags, pAppInfo, layerCount, layerNames, extcount, extNames
        {}, &applicationInfo,
        static_cast<uint32_t>(validationLayers.size()), validationLayers.data(),
        static_cast<uint32_t>(instanceExtensions.size()), instanceExtensions.data());

    // ### DebugInfo: use of StructureChain instead of pNext ###
    // DebugUtils is used to catch errors from the instance
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = makeDebugUtilsMessengerCreateInfoEXT();
    // the StructureChain fills the pNext member of the struct in a typesafe way
    // this is only possible with vulkan-hpp, in plain vulkan there is no typechecking
    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain =
        {instanceCreateInfo, debugCreateInfo};

    if (!enableValidationLayers) // for Release mode
        chain.unlink<vk::DebugUtilsMessengerCreateInfoEXT>();

    // create an Instance
    instance = vk::createInstance(chain.get<vk::InstanceCreateInfo>());

    // update the dispatcher to use instance related extensions
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    if (enableValidationLayers)
        debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(makeDebugUtilsMessengerCreateInfoEXT());
}


std::tuple<uint32_t, uint32_t, uint32_t> getGCTQueues(vk::PhysicalDevice& pDevice) {
    uint32_t gq = -1;
    uint32_t cq = -1;
    uint32_t ocq = -1;
    uint32_t otq = -1;

    using Chain = vk::StructureChain<vk::QueueFamilyProperties2, vk::QueueFamilyCheckpointPropertiesNV>;
    using QFB = vk::QueueFlagBits;
    auto queueFamilyProperties2 = pDevice.getQueueFamilyProperties2<
        Chain, std::allocator<Chain>, vk::DispatchLoaderDynamic>();

    for (uint32_t j = 0; j < queueFamilyProperties2.size(); j++) {
        vk::QueueFamilyProperties const& properties =
            queueFamilyProperties2[static_cast<size_t>(j)].get<vk::QueueFamilyProperties2>().queueFamilyProperties;

        if (properties.queueFlags & QFB::eGraphics)
            if (gq == -1) gq = j;

        if (properties.queueFlags & QFB::eCompute) {
            if (cq == -1) cq = j;
            if (ocq == -1 && !(properties.queueFlags & QFB::eGraphics)) ocq = j;
        }

        if (properties.queueFlags & QFB::eTransfer)
            if (otq == -1 && !(properties.queueFlags & QFB::eCompute || properties.queueFlags & QFB::eGraphics)) otq =
                j;
    }

    if (ocq == -1) ocq = cq;
    if (ocq == gq) ocq = -1;
    return std::tuple<uint32_t, uint32_t, uint32_t>(gq, ocq, otq);
}

void selectPhysicalDevice(vk::Instance& instance, vk::PhysicalDevice& pDevice) {
    // takes the first one
    // Get list of Physical Devices
    // Enumerate Physical devices the vkInstance can access
    std::vector<vk::PhysicalDevice> physDs = instance.enumeratePhysicalDevices();

    const static char* cache_name = "device_selection_cache";
    const static char* recreation_message =
        "To select a new device, delete the file \"device_selection_cache\" in your working directory before executing the framework.";

    std::ifstream ifile(cache_name, std::ios::binary);
    if (ifile.is_open()) {
        DeviceSelectionCache cache;
        ifile.read(reinterpret_cast<char*>(&cache), sizeof(cache));
        ifile.close();
        for (auto physD : physDs) {
            auto props = physD.getProperties2().properties;
            if (props.vendorID == cache.vendorID && props.deviceID == cache.deviceID) {
                std::cout << "Selecting previously selected device: \"" << props.deviceName << "\"" << std::endl;
                std::cout << recreation_message << std::endl;
                pDevice = physD;
                return;
            }
        }
        std::cout << "Previously selected device was not found." << std::endl;
    }
    else {
        std::cout << "No previous device selection found." << std::endl;
    }

    std::cout << "Select one of the available devices:" << std::endl;

    for (int i = 0; i < physDs.size(); i++) {
        auto props = physDs[i].getProperties2().properties;
        std::cout << i << ")\t" << props.deviceName.data() << std::endl;
    }

    uint32_t i;
    while (true) {
        std::cout << "Enter device number: ";
        std::cin >> i;
        if (i < physDs.size()) break;
    }

    auto props = physDs[i].getProperties2().properties;
    DeviceSelectionCache cache;
    cache.vendorID = props.vendorID;
    cache.deviceID = props.deviceID;

    std::ofstream ofile(cache_name, std::ios::out | std::ios::binary);
    ofile.write(reinterpret_cast<const char*>(&cache), sizeof(cache));
    ofile.close();
    std::cout << "Selected device: \"" << props.deviceName.data() << "\"" << std::endl
        << "This device will be automatically selected in the future." << std::endl
        << recreation_message << std::endl;

    pDevice = physDs[i];
}

static std::vector<const char*> requiredDeviceExtensionNames(bool withWindow) {
    std::vector extensionNames(staticExtensionNames);

    // Add VK_KHR_SWAPCHAIN extension if we want to create a window
    if (withWindow) {
        extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    return extensionNames;
}

/*
    The logical device holds the queues and will be used in almost every call from now on
*/
void createLogicalDevice(vk::Instance& instance, vk::PhysicalDevice& pDevice, vk::Device& device, bool withWindow) {
    //first get the queues
    uint32_t gQ, ocQ, otQ;
    std::tie(gQ, ocQ, otQ) = getGCTQueues(pDevice);
    std::vector<vk::DeviceQueueCreateInfo> queuesInfo;
    // flags, queueFamily, queueCount, queuePriority
    float prio = 1.f;
    vk::DeviceQueueCreateInfo graphicsInfo({}, gQ, 1U, &prio);
    queuesInfo.push_back(graphicsInfo);

    if (ocQ != -1) {
        vk::DeviceQueueCreateInfo computeInfo({}, ocQ, 1U, &prio);
        queuesInfo.push_back(computeInfo);
    }

    if (otQ != -1) {
        vk::DeviceQueueCreateInfo transferInfo({}, otQ, 1U, &prio);
        queuesInfo.push_back(transferInfo);
    }
    // {}, queueCreateInfoCount, pQueueCreateInfos, enabledLayerCount, ppEnabledLayerNames, enabledExtensionCount, ppEnabledExtensionNames, pEnabledFeatures

    std::vector extensionNames = requiredDeviceExtensionNames(withWindow);

    // Add VK_KHR_PORTABILITY_SUBSET extension if the device supports it
    auto deviceExtensionProperties = pDevice.enumerateDeviceExtensionProperties();
    for (auto ext : deviceExtensionProperties) {
        if (strcmp(ext.extensionName.data(), VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0) {
            extensionNames.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
            break;
        }
    }

    vk::PhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.fillModeNonSolid = 1;
    deviceFeatures.vertexPipelineStoresAndAtomics = 1;

    vk::DeviceCreateInfo dci({}, queuesInfo.size(), queuesInfo.data(),
                             CAST(validationLayers), validationLayers.data(),
                             CAST(extensionNames), extensionNames.data(), &deviceFeatures); // no extension

    device = pDevice.createDevice(dci);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    setObjectName(device, device, "This is my lovely device !");
}

void createCommandPool(vk::Device& device, vk::CommandPool& commandPool, uint32_t queueIndex) {
    vk::CommandPoolCreateInfo cpi({vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, queueIndex);
    commandPool = device.createCommandPool(cpi);
}

void destroyInstance(vk::Instance& instance, vk::DebugUtilsMessengerEXT& debugUtilsMessenger) {
#ifndef NDEBUG
    instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);
#endif
    instance.destroy();
}

void destroyLogicalDevice(vk::Device& device) {
    device.destroy();
}

void destroyCommandPool(vk::Device& device, vk::CommandPool& commandPool) {
    device.destroyCommandPool(commandPool);
    commandPool = vk::CommandPool();
}

void showAvailableQueues(vk::PhysicalDevice& pDevice, bool diagExt) {
    using Chain = vk::StructureChain<vk::QueueFamilyProperties2, vk::QueueFamilyCheckpointPropertiesNV>;
    auto queueFamilyProperties2 = pDevice.getQueueFamilyProperties2<
        Chain, std::allocator<Chain>, vk::DispatchLoaderDynamic>();

    for (size_t j = 0; j < queueFamilyProperties2.size(); j++) {
        std::cout << "\t"
            << "QueueFamily " << j << "\n";
        vk::QueueFamilyProperties const& properties =
            queueFamilyProperties2[j].get<vk::QueueFamilyProperties2>().queueFamilyProperties;
        std::cout << "\t\t"
            << "QueueFamilyProperties:\n";
        std::cout << "\t\t\t"
            << "queueFlags                  = " << vk::to_string(properties.queueFlags) << "\n";
        std::cout << "\t\t\t"
            << "queueCount                  = " << properties.queueCount << "\n";
        std::cout << "\t\t\t"
            << "timestampValidBits          = " << properties.timestampValidBits << "\n";
        std::cout << "\t\t\t"
            << "minImageTransferGranularity = " << properties.minImageTransferGranularity.width << " x "
            << properties.minImageTransferGranularity.height << " x "
            << properties.minImageTransferGranularity.depth << "\n";
        std::cout << "\n";

        if (diagExt) {
            vk::QueueFamilyCheckpointPropertiesNV const& checkpointProperties =
                queueFamilyProperties2[j].get<vk::QueueFamilyCheckpointPropertiesNV>();
            std::cout << "\t\t"
                << "CheckPointPropertiesNV:\n";
            std::cout << "\t\t\t"
                << "checkpointExecutionStageMask  = "
                << vk::to_string(checkpointProperties.checkpointExecutionStageMask) << "\n";
            std::cout << "\n";
        }
    }
}

void createTimestampQueryPool(vk::Device& device, vk::QueryPool& queryPool, uint32_t queryCount) {
    vk::QueryPoolCreateInfo createInfo({}, vk::QueryType::eTimestamp, queryCount);
    queryPool = device.createQueryPool(createInfo);
}

void destroyQueryPool(vk::Device& device, vk::QueryPool& queryPool) {
    device.destroyQueryPool(queryPool);
    queryPool = vk::QueryPool();
}


void createSwapchain(AppResources& app) {
    // -- CAPABILITIES --
    // Get the surface capabilities for the given surface on the given physical device
    auto capabilities = app.pDevice.getSurfaceCapabilitiesKHR(app.surface);
    // -- FORMATS --
    auto surfaceFormats = app.pDevice.getSurfaceFormatsKHR(app.surface);
    // -- PRESENTATION MODES --
    auto presentModes = app.pDevice.getSurfacePresentModesKHR(app.surface);

    if (surfaceFormats.size() == 0)
        throw std::runtime_error("Surface doesn't support any surface formats!");
    if (presentModes.size() == 0)
        throw std::runtime_error("Surface doesn't support any present modes!");

    vk::SurfaceFormatKHR selectedSurfaceFormat = surfaceFormats[0];
    for (const auto& surfaceFormat : surfaceFormats) {
        if (surfaceFormat.format == vk::Format::eR8G8B8A8Srgb && surfaceFormat.colorSpace ==
            vk::ColorSpaceKHR::eSrgbNonlinear) {
            selectedSurfaceFormat = surfaceFormat;
            break;
        }
    }
    app.surfaceFormat = selectedSurfaceFormat;

    if (!app.pDevice.getSurfaceSupportKHR(app.gQ, app.surface))
        throw std::runtime_error("Graphics Queue doesn't support present to surface!");

    vk::PresentModeKHR selectedPresentMode = presentModes[0];
    // Look for Immediate presentation mode
    for (const auto& presentMode : presentModes) {
        if (presentMode == vk::PresentModeKHR::eImmediate) {
            selectedPresentMode = presentMode;
            break;
        }
    }

    VkExtent2D selectedExtent;
    // If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
    if (capabilities.currentExtent.width != UINT32_MAX) {
        selectedExtent.width = capabilities.currentExtent.width;
        selectedExtent.height = capabilities.currentExtent.height;
    }
    else {
        // If value can vary, need to set manually

        // Get window size
        int width, height;
        glfwGetFramebufferSize(app.window, &width, &height);

        // Create new extent using window size
        selectedExtent = {(uint32_t)width, (uint32_t)height};

        // Surface also defines max and min, so make sure within boundaries by clamping value
        selectedExtent.width = std::clamp(selectedExtent.width, capabilities.minImageExtent.width,
                                          capabilities.maxImageExtent.width);
        selectedExtent.height = std::clamp(selectedExtent.height, capabilities.minImageExtent.height,
                                           capabilities.maxImageExtent.height);
    }

    app.extent = selectedExtent;

    // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
    uint32_t selectedImageCount = capabilities.minImageCount + 1;
    // If imageCount higher than max, then clamp down to max
    selectedImageCount = std::clamp(selectedImageCount, capabilities.minImageCount, capabilities.maxImageCount);

    // Creation information for swap chain
    vk::SwapchainCreateInfoKHR createInfo(
        vk::SwapchainCreateFlagBitsKHR(),
        app.surface, // Swapchain surface
        selectedImageCount, // Minimum images in swapchain
        selectedSurfaceFormat.format, // Swapchain format
        selectedSurfaceFormat.colorSpace, // Swapchain colour space
        selectedExtent, // Swapchain image extents
        1, // Number of layers for each image in chain
        vk::ImageUsageFlagBits::eColorAttachment, // What attachment images will be used as
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::SurfaceTransformFlagBitsKHR::eIdentity, // Transform to perform on swap chain images
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        // How to handle blending images with external graphics (e.g. other windows)
        selectedPresentMode, // Swapchain presentation mode
        false,
        VK_NULL_HANDLE
    );

    app.swapchain = app.device.createSwapchainKHR(createInfo, nullptr);
    // Get swap chain images (first count, then values)
    app.swapchainImages = app.device.getSwapchainImagesKHR(app.swapchain);

    app.swapchainImageViews.resize(0);
    for (auto image : app.swapchainImages) {
        vk::ImageViewCreateInfo createInfo(
            vk::ImageViewCreateFlagBits(),
            image,
            vk::ImageViewType::e2D,
            selectedSurfaceFormat.format,
            {
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
            },
            {
                vk::ImageAspectFlagBits::eColor,
                0, 1, 0, 1
            }
        );

        app.swapchainImageViews.push_back(app.device.createImageView(createInfo));
    }
}

void printDeviceCapabilities(vk::PhysicalDevice& pDevice) {
    //vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();
    std::vector<vk::ExtensionProperties> ext = pDevice.enumerateDeviceExtensionProperties();
    std::vector<vk::LayerProperties> layers = pDevice.enumerateDeviceLayerProperties();
    vk::PhysicalDeviceMemoryProperties memoryProperties = pDevice.getMemoryProperties();
    vk::PhysicalDeviceProperties properties = pDevice.getProperties();
    vk::PhysicalDeviceType dt = properties.deviceType;

    std::cout << "====================" << std::endl
        << "Device Name: " << properties.deviceName << std::endl
        << "Device ID: " << properties.deviceID << std::endl
        << "Device Type: " << vk::to_string(properties.deviceType) << std::endl
        << "Driver Version: " << properties.driverVersion << std::endl
        << "API Version: " << properties.apiVersion << std::endl
        << "====================" << std::endl
        << std::endl;

    bool budgetExt = false;
    bool diagExt = false;
    std::cout << "This device supports the following extensions (" << ext.size() << "): " << std::endl;
    for (vk::ExtensionProperties e : ext) {
        std::cout << std::string(e.extensionName.data()) << std::endl;
        if (std::string(e.extensionName.data()) == VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)
            budgetExt = true;
        if (std::string(e.extensionName.data()) == VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME)
            diagExt = true;
    }

    std::cout << "This device supports the following memory types (" << memoryProperties.memoryTypeCount << "): " <<
        std::endl;
    uint32_t c = 0U;
    for (vk::MemoryType e : memoryProperties.memoryTypes) {
        if (c > memoryProperties.memoryTypeCount)
            break;

        std::cout << e.heapIndex << "\t ";
        std::cout << vk::to_string(e.propertyFlags) << std::endl;
        c++;
    }
    std::cout << "====================" << std::endl
        << std::endl;

    if (budgetExt) {
        std::cout << "This device has the following heaps (" << memoryProperties.memoryHeapCount << "): " << std::endl;
        c = 0U;
        for (vk::MemoryHeap e : memoryProperties.memoryHeaps) {
            if (c > memoryProperties.memoryHeapCount)
                break;

            std::cout << "Size: " << formatSize(e.size) << "\t ";
            std::cout << vk::to_string(e.flags) << std::endl;
            c++;
        }
    }

    std::cout << "====================" << std::endl
        << std::endl
        << "This device has the following layers (" << layers.size() << "): " << std::endl;
    for (vk::LayerProperties l : layers)
        std::cout << std::string(l.layerName.data()) << "\t : " << std::string(l.description.data()) << std::endl;
    std::cout << "====================" << std::endl
        << std::endl;

    showAvailableQueues(pDevice, diagExt);
}
