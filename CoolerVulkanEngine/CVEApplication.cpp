#include "CVEApplication.h"

#include "CVEVulkanInstance.h"

void CVEApplication::Run()
{
    InitApp();
    Update();
}

void CVEApplication::InitApp()
{
    InitWindow();
    CVEVulkanInstance::CreateVulkanInstance(VulkanInstance, DebugMessenger);
    InitPhysicalDevice();
}

void CVEApplication::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    Window = glfwCreateWindow(CVEDefaultWindowParams::DEFAULT_WINDOW_WIDTH, CVEDefaultWindowParams::DEFAULT_WINDOW_HEIGHT, CVEDefaultWindowParams::DEFAULT_WINDOW_TITLE, nullptr, nullptr);
}

void CVEApplication::InitPhysicalDevice()
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

void CVEApplication::InitLogicalDevice()
{
    const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties = PhysicalDevice.getQueueFamilyProperties();
    
    auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](const vk::QueueFamilyProperties& queueFamilyProperties)
    {
        return (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
    });
    assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");
    
    const uint32_t graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
    const float queuePriority = 0.5f;    
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo {.queueFamilyIndex = graphicsIndex,
                                                     .queueCount = 1,
                                                     .pQueuePriorities = &queuePriority};
    
    // Create a chain of feature structures
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain =
    {
        {},                               // vk::PhysicalDeviceFeatures2 (empty for now)
        {.dynamicRendering = true},      // Enable dynamic rendering from Vulkan 1.3
        {.extendedDynamicState = true}   // Enable extended dynamic state from the extension
    };
    
    
    vk::DeviceCreateInfo deviceCreateInfo{.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                          .queueCreateInfoCount = 1,
                                          .pQueueCreateInfos = &deviceQueueCreateInfo,
                                          .enabledExtensionCount = static_cast<uint32_t>(RequiredDeviceExtensions.size()),
                                          .ppEnabledExtensionNames = RequiredDeviceExtensions.data()};
    
    LogicalDevice = vk::raii::Device(PhysicalDevice, deviceCreateInfo);
    GraphicsQueue = vk::raii::Queue(LogicalDevice, graphicsIndex, 0);
}

bool CVEApplication::IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)
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

void CVEApplication::Update()
{
    while (!glfwWindowShouldClose(Window))
    {
        glfwPollEvents();
    }
}

void CVEApplication::TerminateWindow()
{
    glfwDestroyWindow(Window);

    glfwTerminate();
}