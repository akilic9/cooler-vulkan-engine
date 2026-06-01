#pragma once
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN

class CVEVulkanInstance
{
public:
    static void CreateVulkanInstance(vk::raii::Instance& outInstance, vk::raii::DebugUtilsMessengerEXT& outDebugMessenger);
    
private:
    static void InitVulkanInstance(vk::raii::Instance& instance);
    static void InitDebugMessenger(const vk::raii::Instance& instance, vk::raii::DebugUtilsMessengerEXT& outDebugMessenger);
    
    static std::vector<char const*> GetRequiredLayers();
    static std::vector<const char*> GetRequiredExtensions();
    
    static void CheckLayersSupport(const vk::raii::Context& instanceContext, const std::vector<const char*>& inLayers);
    static void CheckExtensionsSupport(const vk::raii::Context& instanceContext, const std::vector<const char*>& inExtensions);
    
    static void PrintExtensions(const std::vector<vk::ExtensionProperties>& inAvailableExtensions, const std::vector<const char*>& inRequiredExtensions);
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    
    static constexpr std::vector<const char*> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    
#ifdef NDEBUG
    static constexpr bool bValidationLayersEnabled = false;
#else
    static constexpr bool bValidationLayersEnabled = true;
#endif
};
