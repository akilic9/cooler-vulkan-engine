#pragma once
#include <array>
#include <vector>
#include <vulkan/vulkan_core.h>

class CVEInstanceUtil
{
public:
    CVEInstanceUtil() = delete;
    
    static constexpr std::array RequiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    
    static constexpr std::array ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    
#ifdef NDEBUG
    static constexpr bool bValidationLayersEnabled = false;
#else
    static constexpr bool bValidationLayersEnabled = true;
#endif //NDEBUG
    
    static std::vector<char const*> GetRequiredLayers();
    static std::vector<const char*> GetRequiredExtensions();
    
    static void CheckLayersSupport(const std::vector<char const*>& reqLayers);
    static void CheckExtensionsSupport(const std::vector<char const*>& reqExtensions);
    
    static void PrintExtensions(const std::vector<VkExtensionProperties>& inAvailableExtensions, const std::vector<const char*>& inRequiredExtensions);
};
