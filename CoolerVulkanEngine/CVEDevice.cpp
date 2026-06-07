#include "CVEDevice.h"

#include <glfw3.h>
#include <iostream>

#include "CVEWindow.h"

CVEDevice::CVEDevice(CVEWindow& inWindow)
    : Window(inWindow)
{
    CreateVulkanInstance();
    CreateDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
}

CVEDevice::~CVEDevice()
{
}

void CVEDevice::CreateVulkanInstance()
{
    constexpr vk::ApplicationInfo appInfo{.pApplicationName = "CoolerVulkanEngine",
                                          .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .pEngineName = "No Engine",
                                          .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .apiVersion = vk::ApiVersion14};
    
    const std::vector<const char*>& requiredLayers = GetRequiredLayers();
    CheckLayersSupport(requiredLayers);
    
    const std::vector<const char*>& requiredExtensions = GetRequiredExtensions();
    CheckExtensionsSupport(requiredExtensions);
    
    vk::InstanceCreateInfo createInfo{.pApplicationInfo = &appInfo,
                                      .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
                                      .ppEnabledLayerNames = requiredLayers.data(),
                                      .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
                                      .ppEnabledExtensionNames = requiredExtensions.data()};
    
    VulkanInstance = vk::raii::Instance(VulkanInstanceContext, createInfo);
}

void CVEDevice::CreateDebugMessenger()
{
    if (!bValidationLayersEnabled)
    {
        return;
    }
    
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                          .messageType     = messageTypeFlags,
                                                                          .pfnUserCallback = &DebugCallback};
    
    DebugMessenger = VulkanInstance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

std::vector<char const*> CVEDevice::GetRequiredLayers()
{
    std::vector<char const*> requiredLayers;

    if (bValidationLayersEnabled)
    {
        requiredLayers.assign(ValidationLayers.begin(), ValidationLayers.end());
    }
    
    return requiredLayers;
}

std::vector<char const*> CVEDevice::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if (bValidationLayersEnabled)
    {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    
    return extensions;
}

void CVEDevice::CheckLayersSupport(const std::vector<char const*>& inLayers)
{
    const std::vector<vk::LayerProperties>& layerProperties = VulkanInstanceContext.enumerateInstanceLayerProperties();
    
    //TODO: these nested lambdas may be hard to read
    auto unsupportedLayerIt = std::ranges::find_if(inLayers, [&layerProperties](const char* requiredLayer)
    {
        return std::ranges::none_of(layerProperties, [requiredLayer](const vk::LayerProperties& layerProperty)
        {
            return strcmp(layerProperty.layerName, requiredLayer) == 0;
        });
    });
    
    if (unsupportedLayerIt != inLayers.end())
    {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }
}

void CVEDevice::CheckExtensionsSupport(const std::vector<char const*>& inExtensions)
{
    const std::vector<vk::ExtensionProperties>& extensionProperties = VulkanInstanceContext.enumerateInstanceExtensionProperties();
    
#ifdef _DEBUG
    PrintExtensions(extensionProperties, inExtensions);
#endif // _DEBUG

    //TODO: these nested lambdas may be hard to read
    auto unsupportedPropertyIt = std::ranges::find_if(inExtensions, [&extensionProperties](const char* requiredExtension)
    {
        return std::ranges::none_of(extensionProperties, [requiredExtension](const vk::ExtensionProperties& extensionProperty)
        {
            return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
        });
    });
    
    if (unsupportedPropertyIt != inExtensions.end())
    {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }
}

void CVEDevice::PrintExtensions(const std::vector<vk::ExtensionProperties>& inAvailableExtensions, const std::vector<char const*>& inRequiredExtensions)
{
    std::cout << "Available extensions:\n";

    for (const vk::ExtensionProperties& availableExtension : inAvailableExtensions)
    {
        std::cout << '\t' << availableExtension.extensionName << '\n';
    }
    
    std::cout << "Required extensions:\n";
    for (const char* requiredExtension : inRequiredExtensions)
    {
        std::cout << '\t' << requiredExtension << '\n';
    }
}

vk::Bool32 CVEDevice::DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
                                    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "Validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

    return vk::False;
}

void CVEDevice::PickPhysicalDevice()
{
    const std::vector<vk::raii::PhysicalDevice>& physicalDevices = VulkanInstance.enumeratePhysicalDevices();
    const auto deviceIt = std::ranges::find_if(physicalDevices, [&](const vk::raii::PhysicalDevice& physicalDevice)
    {
        return IsDeviceSuitable(physicalDevice);
    });
    
    if (deviceIt == physicalDevices.end())
    {
        throw std::runtime_error( "failed to find a suitable GPU!" );
    }
    
    PhysicalDevice = *deviceIt;
}

void CVEDevice::CreateLogicalDevice()
{
    const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties = PhysicalDevice.getQueueFamilyProperties();

    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) && PhysicalDevice.getSurfaceSupportKHR(qfpIndex, *Surface))
        {
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }
    
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain =
    {
        {},
        {.dynamicRendering = true},
        {.extendedDynamicState = true}
    };
    
    const float queuePriority = 0.5f;    
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo {.queueFamilyIndex = queueIndex,
                                                     .queueCount = 1,
                                                     .pQueuePriorities = &queuePriority};
    
    vk::DeviceCreateInfo deviceCreateInfo{.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                          .queueCreateInfoCount = 1,
                                          .pQueueCreateInfos = &deviceQueueCreateInfo,
                                          .enabledExtensionCount = static_cast<uint32_t>(RequiredDeviceExtensions.size()),
                                          .ppEnabledExtensionNames = RequiredDeviceExtensions.data()};
    
    LogicalDevice = vk::raii::Device(PhysicalDevice, deviceCreateInfo);
    GraphicsQueue = vk::raii::Queue(LogicalDevice, queueIndex, 0);
}

bool CVEDevice::IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)
{
    const bool bSupportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

    const std::vector<vk::QueueFamilyProperties>& queueFamilies = physicalDevice.getQueueFamilyProperties();
    const bool bSupportsGraphics = std::ranges::any_of(queueFamilies, [](const vk::QueueFamilyProperties& queueFamilyProperty)
    {
        return !!(queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics); // ??
    });

    const std::vector<vk::ExtensionProperties>& availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    
    //TODO: these nested lambdas may be hard to read
    const bool bSupportsAllRequiredExtensions = std::ranges::all_of(RequiredDeviceExtensions, [&availableDeviceExtensions](const char* requiredDeviceExtension)
    {
        return std::ranges::any_of(availableDeviceExtensions, [requiredDeviceExtension](const vk::ExtensionProperties& availableDeviceExtension)
        {
            return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
        });
    });
    
    // what??
    auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool bSupportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                     features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
    
    return bSupportsVulkan1_3 && bSupportsGraphics && bSupportsAllRequiredExtensions && bSupportsRequiredFeatures;
}

void CVEDevice::CreateSurface()
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*VulkanInstance, Window.GetGLFWWindow(), nullptr, &surface) != 0)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    Surface = vk::raii::SurfaceKHR(VulkanInstance, surface);
}

void CVEDevice::CreateSwapChain()
{
    const vk::SurfaceCapabilitiesKHR& surfaceCapabilities = PhysicalDevice.getSurfaceCapabilitiesKHR(*Surface);
    Extent = ChooseExtent(surfaceCapabilities);
    uint32_t minImageCount = ChooseMinImageCount(surfaceCapabilities);

    const std::vector<vk::SurfaceFormatKHR>& availableFormats = PhysicalDevice.getSurfaceFormatsKHR(*Surface);
    SurfaceFormat = ChooseSurfaceFormat(availableFormats);
    
    const std::vector<vk::PresentModeKHR>& availablePresentModes = PhysicalDevice.getSurfacePresentModesKHR(*Surface);
    
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{.surface          = *Surface,
                                                   .minImageCount    = minImageCount,
                                                   .imageFormat      = SurfaceFormat.format,
                                                   .imageColorSpace  = SurfaceFormat.colorSpace,
                                                   .imageExtent      = Extent,
                                                   .imageArrayLayers = 1,
                                                   .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
                                                   .imageSharingMode = vk::SharingMode::eExclusive,
                                                   .preTransform     = surfaceCapabilities.currentTransform,
                                                   .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                   .presentMode      = ChoosePresentMode(availablePresentModes),
                                                   .clipped          = true};
    
    swapChainCreateInfo.oldSwapchain = nullptr;
    
    SwapChain = vk::raii::SwapchainKHR(LogicalDevice, swapChainCreateInfo);
    SwapChainImages = SwapChain.getImages();
}

vk::SurfaceFormatKHR CVEDevice::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    const auto formatItr = std::ranges::find_if(availableFormats, [](const vk::SurfaceFormatKHR& format)
    {
        return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    
    return formatItr != availableFormats.end() ? *formatItr : availableFormats[0];
}

vk::PresentModeKHR CVEDevice::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    const bool bSupportsFIFO = std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR& presentMode)
    {
        return presentMode == vk::PresentModeKHR::eFifo;
    });
    
    assert(bSupportsFIFO);
    
    const bool bSupportsMailbox = std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR& presentMode)
    {
        return vk::PresentModeKHR::eMailbox == presentMode;
    });
    
    return bSupportsMailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

vk::Extent2D CVEDevice::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    
    int width, height;
    glfwGetFramebufferSize(Window.GetGLFWWindow(), &width, &height);

    return vk::Extent2D
    {
        .width = std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t CVEDevice::ChooseMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    uint32_t minImageCount = std::max(3u, capabilities.minImageCount);
    if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < minImageCount))
    {
        minImageCount = capabilities.maxImageCount;
    }
    return minImageCount;
}

void CVEDevice::CreateImageViews()
{
    assert(SwapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo{.viewType = vk::ImageViewType::e2D,
                                                .format = SurfaceFormat.format,
                                                .components = {.r = vk::ComponentSwizzle::eIdentity,
                                                                  .g = vk::ComponentSwizzle::eIdentity,
                                                                  .b = vk::ComponentSwizzle::eIdentity,
                                                                  .a = vk::ComponentSwizzle::eIdentity},
                                                .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                                        .levelCount = 1,
                                                                        .layerCount = 1}};
    
    for (const vk::Image& image : SwapChainImages)
    {
        imageViewCreateInfo.image = image;
        SwapChainImageViews.emplace_back(LogicalDevice, imageViewCreateInfo);
    }
}
