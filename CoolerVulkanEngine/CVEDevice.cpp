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
    CreateCommandPool();
}

CVEDevice::~CVEDevice()
{
}

const vk::raii::Device& CVEDevice::GetLogicalDevice() const
{
    return LogicalDevice;
}

vk::raii::SurfaceKHR& CVEDevice::GetSurface()
{
    return Surface;
}

CVESwapChainSupportDetails CVEDevice::GetSwapChainSupportDetails() const
{
    CVESwapChainSupportDetails details;    
    details.SurfaceCapabilities   = PhysicalDevice.getSurfaceCapabilitiesKHR(*Surface);    
    details.AvailableFormats      = PhysicalDevice.getSurfaceFormatsKHR(*Surface);
    details.AvailablePresentModes = PhysicalDevice.getSurfacePresentModesKHR(*Surface);
    return details;
}

const vk::raii::CommandPool& CVEDevice::GetCommandPool() const
{
    return CommandPool;
}

vk::raii::Queue& CVEDevice::GetGraphicsQueue()
{
    return GraphicsQueue;
}

uint32_t CVEDevice::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    const vk::PhysicalDeviceMemoryProperties& memProperties = PhysicalDevice.getMemoryProperties();
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CVEDevice::CreateBuffer(vk::DeviceSize size,
                                                                            vk::BufferUsageFlags usageFlags,
                                                                            vk::MemoryPropertyFlags propertyFlags)
{
    vk::BufferCreateInfo bufferCreateInfo {
        .size        = size,
        .usage       = usageFlags,
        .sharingMode = vk::SharingMode::eExclusive};

    vk::raii::Buffer buffer = vk::raii::Buffer(LogicalDevice, bufferCreateInfo);

    vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo {
        .allocationSize  = memoryRequirements.size,
        .memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, propertyFlags)};

    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(LogicalDevice, memoryAllocateInfo);

    buffer.bindMemory(*bufferMemory, 0);

    return {std::move(buffer), std::move(bufferMemory)};
}

vk::raii::CommandBuffer CVEDevice::BeginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo {
        .commandPool        = CommandPool,
        .level              = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};

    vk::raii::CommandBuffer commandBuffer = std::move(LogicalDevice.allocateCommandBuffers(allocInfo).front());
    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return commandBuffer; // why does the original has std::move here?
}

void CVEDevice::EndSingleTimeCommands(const vk::raii::CommandBuffer& commandBuffer)
{
    commandBuffer.end();
    
    vk::SubmitInfo submitInfo {.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    
    GraphicsQueue.submit(submitInfo, nullptr);
    GraphicsQueue.waitIdle();
}

void CVEDevice::CopyBuffer(const vk::raii::Buffer& source, const vk::raii::Buffer& destination, vk::DeviceSize size)
{
    const vk::raii::CommandBuffer& commandCopyBuffer = BeginSingleTimeCommands();
    vk::BufferCopy bufferCopy {.srcOffset = 0, .dstOffset = 0, .size = size};
    commandCopyBuffer.copyBuffer(*source, *destination, bufferCopy);
    EndSingleTimeCommands(commandCopyBuffer);
}

void CVEDevice::CopyBufferToImage(const vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer,
                                  vk::raii::Image& image, uint32_t width, uint32_t height)
{
    vk::BufferImageCopy region {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1},
        .imageOffset       = {0, 0, 0},
        .imageExtent       = {width, height, 1}};
    
    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
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
    
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan11Features, 
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},
        {.shaderDrawParameters = true},
        {.synchronization2 = true,
            .dynamicRendering = true},
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
        return !!(queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics);
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

    auto features = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                                                               vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    
    bool bSupportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                     features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                     features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
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