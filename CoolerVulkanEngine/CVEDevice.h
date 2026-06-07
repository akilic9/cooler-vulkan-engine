#pragma once
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <vector>

class CVEWindow;

class CVEDevice
{
public:
    CVEDevice(CVEWindow& inWindow);
    ~CVEDevice();
    
    CVEDevice(const CVEDevice&) = delete;
    CVEDevice& operator=(const CVEDevice&) = delete;
    CVEDevice(CVEDevice&&) = delete;
    CVEDevice& operator=(CVEDevice&&) = delete;

private:
    /// INSTANCE CREATION
    void CreateVulkanInstance();
    void CreateDebugMessenger();
    
    std::vector<char const*> GetRequiredLayers();
    std::vector<const char*> GetRequiredExtensions();
    
    void CheckLayersSupport(const std::vector<const char*>& inLayers);
    void CheckExtensionsSupport(const std::vector<const char*>& inExtensions);
    
    static void PrintExtensions(const std::vector<vk::ExtensionProperties>& inAvailableExtensions, const std::vector<const char*>& inRequiredExtensions);
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    /// INSTANCE CREATION END
    
    /// PHYSICAL DEVICE
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    bool IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);
    
    void CreateSurface();
    /// PHYSICAL DEVICE END
    
    /// SWAP CHAIN
    void CreateSwapChain();
    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    uint32_t ChooseMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);
    
    void CreateImageViews();
    /// SWAP CHAIN END
    
    /// INSTANCE CREATION VARS
    std::vector<const char*> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    
#ifdef NDEBUG
    static constexpr bool bValidationLayersEnabled = false;
#else
    static constexpr bool bValidationLayersEnabled = true;
#endif
    
    std::vector<const char*> RequiredDeviceExtensions = {vk::KHRSwapchainExtensionName};
    
    vk::raii::Context VulkanInstanceContext;
    vk::raii::Instance VulkanInstance = nullptr;
    vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;
    /// INSTANCE CREATION VARS END
    
    /// PHYSICAL DEVICE VARS
    CVEWindow& Window;
    vk::raii::PhysicalDevice PhysicalDevice = nullptr;
    vk::raii::Device LogicalDevice = nullptr;
    vk::raii::Queue GraphicsQueue = nullptr;
    
    vk::raii::SurfaceKHR Surface = nullptr;
    /// PHYSICAL DEVICE VARS END
    
    /// SWAP CHAIN VARS
    vk::raii::SwapchainKHR SwapChain = nullptr;
    std::vector<vk::Image> SwapChainImages;
    vk::Extent2D Extent;
    vk::SurfaceFormatKHR SurfaceFormat;
    std::vector<vk::raii::ImageView> SwapChainImageViews;
    ///  SWAP CHAIN VARS END
};
