#include "CVEDevice.h"

#include <algorithm>
#include <glfw3.h>
#include <iostream>
#include <fstream>
#include <string>

#include "CVEInstanceUtil.h"
#include "CVEWindow.h"

CVEDevice::CVEDevice(CVEWindow& inWindow)
    : Window(inWindow)
{
    CreateVulkanInstance();
    CreateDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateCommandPool();
}

CVEDevice::~CVEDevice()
{
    vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);
    vkDestroyDevice(LogicalDevice, nullptr);
    
    if (CVEInstanceUtil::bValidationLayersEnabled)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
    
        if (func != nullptr)
        {
            func(VulkanInstance, DebugMessenger, nullptr);
        }
    }
    
    vkDestroySurfaceKHR(VulkanInstance, Surface, nullptr);
    vkDestroyInstance(VulkanInstance, nullptr);
}

const VkDevice& CVEDevice::GetLogicalDevice() const
{
    return LogicalDevice;
}

VkSurfaceKHR& CVEDevice::GetSurface()
{
    return Surface;
}

CVESwapChainSupportDetails CVEDevice::GetSwapChainSupportDetails() const
{
    CVESwapChainSupportDetails details;    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &details.SurfaceCapabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &formatCount, nullptr);    
    if (formatCount != 0)
    {
        details.AvailableFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &formatCount, details.AvailableFormats.data());
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, nullptr);    
    if (presentModeCount != 0)
    {
        details.AvailablePresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, details.AvailablePresentModes.data());
    }
    
    return details;
}

const VkCommandPool& CVEDevice::GetCommandPool() const
{
    return CommandPool;
}

VkQueue& CVEDevice::GetGraphicsQueue()
{
    return GraphicsQueue;
}

uint32_t CVEDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void CVEDevice::CreateBuffer(VkDeviceSize size,
                             VkBufferUsageFlags usageFlags,
                             VkMemoryPropertyFlags propertyFlags,
                             VkBuffer& outBuffer,
                             VkDeviceMemory& outBufferMemory)
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size        = size;
    bufferCreateInfo.usage       = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(LogicalDevice, &bufferCreateInfo, nullptr, &outBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(LogicalDevice, outBuffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize  = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);

    if (vkAllocateMemory(LogicalDevice, &memoryAllocateInfo, nullptr, &outBufferMemory) != VK_SUCCESS)
    {
        vkDestroyBuffer(LogicalDevice, outBuffer, nullptr);
        throw std::runtime_error("Failed to allocate buffer memory!");
    }
    
    vkBindBufferMemory(LogicalDevice, outBuffer, outBufferMemory, 0);
}

VkCommandBuffer CVEDevice::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = CommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(LogicalDevice, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void CVEDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;
    
    vkQueueSubmit(GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(GraphicsQueue);
    
    vkFreeCommandBuffers(LogicalDevice, CommandPool, 1, &commandBuffer);
}

void CVEDevice::CopyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size)
{
    const VkCommandBuffer& commandCopyBuffer = BeginSingleTimeCommands();
    VkBufferCopy bufferCopy {.srcOffset = 0, .dstOffset = 0, .size = size};
    vkCmdCopyBuffer(commandCopyBuffer, source, destination, 1, &bufferCopy);
    EndSingleTimeCommands(commandCopyBuffer);
}

void CVEDevice::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer,
                                  VkImage image, uint32_t width, uint32_t height)
{
    VkImageSubresourceLayers imageSubresource{};
    imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresource.mipLevel       = 0;
    imageSubresource.baseArrayLayer = 0;
    imageSubresource.layerCount     = 1;
    
    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource  = imageSubresource;
    region.imageOffset       = {0, 0, 0};
    region.imageExtent       = {width, height, 1};
    
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CVEDevice::CreateVulkanInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "CoolerVulkanEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_4;
    
    const std::vector<const char*>& requiredLayers = CVEInstanceUtil::GetRequiredLayers();
    CVEInstanceUtil::CheckLayersSupport(requiredLayers);
    
    const std::vector<const char*>& requiredExtensions = CVEInstanceUtil::GetRequiredExtensions();
    CVEInstanceUtil::CheckExtensionsSupport(requiredExtensions);
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames     = requiredLayers.data();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    
    if (vkCreateInstance(&createInfo, nullptr, &VulkanInstance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance!");
    }
}

void CVEDevice::CreateDebugMessenger()
{
    if (!CVEInstanceUtil::bValidationLayersEnabled)
    {
        return;
    }
    
    VkDebugUtilsMessageSeverityFlagsEXT severityFlags(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
    
    VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT);
    
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType     = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &DebugCallback;
    
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VulkanInstance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        func(VulkanInstance, &debugUtilsMessengerCreateInfoEXT, nullptr, &DebugMessenger);
    }
    else
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL CVEDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "Validation layer: type " << std::to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void CVEDevice::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, nullptr);
    
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, physicalDevices.data());
    
    const auto deviceIt = std::ranges::find_if(physicalDevices, [&](const VkPhysicalDevice& physicalDevice)
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
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, qfpIndex, Surface, &presentSupport);
        
        if ((queueFamilyProperties[qfpIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
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
    
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extDynamicStateFeatures{};
    extDynamicStateFeatures.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extDynamicStateFeatures.extendedDynamicState = VK_TRUE;

    VkPhysicalDeviceVulkan13Features vk13Features{};
    vk13Features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vk13Features.pNext            = &extDynamicStateFeatures;
    vk13Features.synchronization2 = VK_TRUE;
    vk13Features.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceVulkan11Features vk11Features{};
    vk11Features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vk11Features.pNext                = &vk13Features;
    vk11Features.shaderDrawParameters = VK_TRUE;

    VkPhysicalDeviceFeatures2 featureChain{};
    featureChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    featureChain.pNext = &vk11Features;
    
    const float queuePriority = 0.5f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
    deviceQueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
    deviceQueueCreateInfo.queueCount       = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext                   = &featureChain;
    deviceCreateInfo.queueCreateInfoCount    = 1;
    deviceCreateInfo.pQueueCreateInfos       = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(CVEInstanceUtil::RequiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = CVEInstanceUtil::RequiredDeviceExtensions.data();
    
    if (vkCreateDevice(PhysicalDevice, &deviceCreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }
    
    vkGetDeviceQueue(LogicalDevice, QueueFamilyIndex, 0, &GraphicsQueue);
}

// TODO: Split
bool CVEDevice::IsDeviceSuitable(const VkPhysicalDevice& physicalDevice)
{
    VkPhysicalDeviceProperties2 deviceProperties{};
    deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    
    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);
    
    const bool bSupportsVulkan1_3 = deviceProperties.properties.apiVersion >= VK_API_VERSION_1_3;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    
    const bool bSupportsGraphics = std::ranges::any_of(queueFamilies, [](const VkQueueFamilyProperties& queueFamilyProperty)
    {
        return !!(queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT);
    });

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    
    //TODO: these nested lambdas may be hard to read
    const bool bSupportsAllRequiredExtensions = std::ranges::all_of(CVEInstanceUtil::RequiredDeviceExtensions, [&availableExtensions](const char* requiredDeviceExtension)
    {
        return std::ranges::any_of(availableExtensions, [requiredDeviceExtension](const VkExtensionProperties& availableDeviceExtension)
        {
            return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
        });
    });

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extDynStateFeatures{};
    extDynStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

    VkPhysicalDeviceVulkan13Features vk13Features{};
    vk13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vk13Features.pNext = &extDynStateFeatures;

    VkPhysicalDeviceVulkan11Features vk11Features{};
    vk11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vk11Features.pNext = &vk13Features;

    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &vk11Features;
    
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features);
    
    bool bSupportsRequiredFeatures = vk11Features.shaderDrawParameters &&
                                     vk13Features.dynamicRendering     &&
                                     vk13Features.synchronization2     &&
                                     extDynStateFeatures.extendedDynamicState;
    
    return bSupportsVulkan1_3 && bSupportsGraphics && bSupportsAllRequiredExtensions && bSupportsRequiredFeatures;
}

void CVEDevice::CreateSurface()
{
    if (glfwCreateWindowSurface(VulkanInstance, Window.GetGLFWWindow(), nullptr, &Surface) != 0)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void CVEDevice::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.pNext            = nullptr;
    poolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
    
    if (vkCreateCommandPool(LogicalDevice, &poolCreateInfo, nullptr, &CommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}