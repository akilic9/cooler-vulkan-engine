#include "CVEInstanceUtil.h"

#include <algorithm>
#include <glfw3.h>
#include <iostream>

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
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

void CVEInstanceUtil::CheckLayersSupport(const std::vector<char const*>& reqLayers)
{    
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());
    
    //TODO: these nested lambdas may be hard to read
    auto unsupportedLayerIt = std::ranges::find_if(reqLayers, [&layerProperties](const char* requiredLayer)
    {
        return std::ranges::none_of(layerProperties, [requiredLayer](const VkLayerProperties& layerProperty)
        {
            return strcmp(layerProperty.layerName, requiredLayer) == 0;
        });
    });
    
    if (unsupportedLayerIt != reqLayers.end())
    {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }
}

void CVEInstanceUtil::CheckExtensionsSupport(const std::vector<char const*>& reqExtensions)
{    
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());
    
#ifdef _DEBUG
    PrintExtensions(extensionProperties, reqExtensions);
#endif // _DEBUG

    //TODO: these nested lambdas may be hard to read
    auto unsupportedPropertyIt = std::ranges::find_if(reqExtensions, [&extensionProperties](const char* requiredExtension)
    {
        return std::ranges::none_of(extensionProperties, [requiredExtension](const VkExtensionProperties& extensionProperty)
        {
            return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
        });
    });
    
    if (unsupportedPropertyIt != reqExtensions.end())
    {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }
}

void CVEInstanceUtil::PrintExtensions(const std::vector<VkExtensionProperties>& inAvailableExtensions, const std::vector<char const*>& inRequiredExtensions)
{
    std::cout << "Available extensions:\n";

    for (const VkExtensionProperties& availableExtension : inAvailableExtensions)
    {
        std::cout << '\t' << availableExtension.extensionName << '\n';
    }
    
    std::cout << "Required extensions:\n";
    for (const char* requiredExtension : inRequiredExtensions)
    {
        std::cout << '\t' << requiredExtension << '\n';
    }
}