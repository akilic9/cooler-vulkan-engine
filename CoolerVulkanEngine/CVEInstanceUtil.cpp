#include "CVEInstanceUtil.h"
#include <glfw3.h>
#include <iostream>

vk::raii::Instance CVEInstanceUtil::CreateVulkanInstance(const vk::raii::Context& instanceContext)
{
    constexpr vk::ApplicationInfo appInfo {
        .pApplicationName   = "CoolerVulkanEngine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14};
    
    const std::vector<const char*>& requiredLayers = GetRequiredLayers();
    CheckLayersSupport(instanceContext, requiredLayers);
    
    const std::vector<const char*>& requiredExtensions = GetRequiredExtensions();
    CheckExtensionsSupport(instanceContext, requiredExtensions);
    
    vk::InstanceCreateInfo createInfo {
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames     = requiredLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()};
    
    return vk::raii::Instance(instanceContext, createInfo);
}

std::vector<char const*> CVEInstanceUtil::GetRequiredLayers()
{
    std::vector<char const*> requiredLayers;

    if (bValidationLayersEnabled)
    {
        requiredLayers.assign(ValidationLayers.begin(), ValidationLayers.end());
    }
    
    return requiredLayers;
}

std::vector<char const*> CVEInstanceUtil::GetRequiredExtensions()
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

void CVEInstanceUtil::CheckLayersSupport(const vk::raii::Context& instanceContext, const std::vector<char const*>& inLayers)
{
    const std::vector<vk::LayerProperties>& layerProperties = instanceContext.enumerateInstanceLayerProperties();
    
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

void CVEInstanceUtil::CheckExtensionsSupport(const vk::raii::Context& instanceContext, const std::vector<char const*>& inExtensions)
{
    const std::vector<vk::ExtensionProperties>& extensionProperties = instanceContext.enumerateInstanceExtensionProperties();
    
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

void CVEInstanceUtil::PrintExtensions(const std::vector<vk::ExtensionProperties>& inAvailableExtensions, const std::vector<char const*>& inRequiredExtensions)
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