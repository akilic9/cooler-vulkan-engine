#include "CVEDevice.h"

#include <glfw3.h>
#include <iostream>
#include <fstream>

#include "CVEInstanceUtil.h"
#include "CVEWindow.h"

CVEDevice::CVEDevice(CVEWindow& inWindow)
    : Window(inWindow)
{
    VulkanInstance = CVEInstanceUtil::CreateVulkanInstance(VulkanInstanceContext);
    CreateDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
}

CVEDevice::~CVEDevice()
{
}

const vk::raii::Device& CVEDevice::GetLogicalDevice() const
{
    return LogicalDevice;
}

const vk::raii::SurfaceKHR& CVEDevice::GetSurface() const
{
    return Surface;
}

CVESwapChainSupportDetails CVEDevice::GetSwapChainSupportDetails() const
{
    CVESwapChainSupportDetails details;    
    details.SurfaceCapabilities = PhysicalDevice.getSurfaceCapabilitiesKHR(*Surface);    
    details.AvailableFormats = PhysicalDevice.getSurfaceFormatsKHR(*Surface);
    details.AvailablePresentModes = PhysicalDevice.getSurfacePresentModesKHR(*Surface);
    return details;
}

const vk::raii::CommandPool& CVEDevice::GetCommandPool() const
{
    return CommandPool;
}

void CVEDevice::CreateDebugMessenger()
{
    if (!CVEInstanceUtil::bValidationLayersEnabled)
    {
        return;
    }
    
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT {
        .messageSeverity = severityFlags,
        .messageType     = messageTypeFlags,
        .pfnUserCallback = &DebugCallback};
    
    DebugMessenger = VulkanInstance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
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
        throw std::runtime_error( "Failed to find a suitable GPU!" );
    }
    
    PhysicalDevice = *deviceIt;
}

void CVEDevice::FindQueueFamilyIndex()
{
    const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties = PhysicalDevice.getQueueFamilyProperties();

    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) && PhysicalDevice.getSurfaceSupportKHR(qfpIndex, *Surface))
        {
            QueueFamilyIndex = qfpIndex;
            break;
        }
    }

    if (QueueFamilyIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present, terminating...");
    }
}

void CVEDevice::CreateLogicalDevice()
{
    FindQueueFamilyIndex();
    
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},
        {.dynamicRendering     = true},
        {.extendedDynamicState = true}};
    
    const float queuePriority = 0.5f;    
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo {
        .queueFamilyIndex = QueueFamilyIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority};
    
    vk::DeviceCreateInfo deviceCreateInfo {
        .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(CVEInstanceUtil::RequiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = CVEInstanceUtil::RequiredDeviceExtensions.data()};
    
    LogicalDevice = vk::raii::Device(PhysicalDevice, deviceCreateInfo);
    GraphicsQueue = vk::raii::Queue(LogicalDevice, QueueFamilyIndex, 0);
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
    const bool bSupportsAllRequiredExtensions = std::ranges::all_of(CVEInstanceUtil::RequiredDeviceExtensions, [&availableDeviceExtensions](const char* requiredDeviceExtension)
    {
        return std::ranges::any_of(availableDeviceExtensions, [requiredDeviceExtension](const vk::ExtensionProperties& availableDeviceExtension)
        {
            return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
        });
    });

    auto features = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool bSupportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                     features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
    
    return bSupportsVulkan1_3 && bSupportsGraphics && bSupportsAllRequiredExtensions && bSupportsRequiredFeatures;
}

void CVEDevice::CreateSurface()
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*VulkanInstance, Window.GetGLFWWindow(), nullptr, &surface) != 0)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
    Surface = vk::raii::SurfaceKHR(VulkanInstance, surface);
}

void CVEDevice::CreateCommandPool()
{
    vk::CommandPoolCreateInfo poolCreateInfo {
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = QueueFamilyIndex};
    
    CommandPool = vk::raii::CommandPool(LogicalDevice, poolCreateInfo);
}
