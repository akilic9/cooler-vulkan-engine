#pragma once
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

class CVEInstanceUtil
{
public:
    CVEInstanceUtil() = delete;
    
    static vk::raii::Instance CreateVulkanInstance(const vk::raii::Context& instanceContext);
    
    static constexpr std::array RequiredDeviceExtensions = {vk::KHRSwapchainExtensionName};
    
    static constexpr std::array ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    
#ifdef NDEBUG
    static constexpr bool bValidationLayersEnabled = false;
#else
    static constexpr bool bValidationLayersEnabled = true;
#endif //NDEBUG
    
private:    
    static std::vector<char const*> GetRequiredLayers();
    static std::vector<const char*> GetRequiredExtensions();
    
    static void CheckLayersSupport(const vk::raii::Context& instanceContext, const std::vector<const char*>& inLayers);
    static void CheckExtensionsSupport(const vk::raii::Context& instanceContext, const std::vector<const char*>& inExtensions);
    
    static void PrintExtensions(const std::vector<vk::ExtensionProperties>& inAvailableExtensions, const std::vector<const char*>& inRequiredExtensions);
};
