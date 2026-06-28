#pragma once
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <vector>

class CVEWindow;

struct CVESwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR SurfaceCapabilities;
    std::vector<vk::SurfaceFormatKHR> AvailableFormats;
    std::vector<vk::PresentModeKHR> AvailablePresentModes;
};

class CVEDevice
{
public:
    CVEDevice(CVEWindow& inWindow);
    ~CVEDevice();
    
    CVEDevice(const CVEDevice&) = delete;
    CVEDevice& operator=(const CVEDevice&) = delete;
    CVEDevice(CVEDevice&&) = delete;
    CVEDevice& operator=(CVEDevice&&) = delete;
    
    const vk::raii::Device& GetLogicalDevice() const;
    vk::raii::SurfaceKHR& GetSurface();
    CVESwapChainSupportDetails GetSwapChainSupportDetails() const;
    const vk::raii::CommandPool& GetCommandPool() const;
    vk::raii::Queue& GetGraphicsQueue();
    
private:
    void CreateDebugMessenger();
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, 
                                                      const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    
    void PickPhysicalDevice();
    void FindQueueFamilyIndex();
    void CreateLogicalDevice();
    bool IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);
    
    void CreateSurface();
    
    void CreateCommandPool();
    
    vk::raii::Context VulkanInstanceContext;
    vk::raii::Instance VulkanInstance = nullptr;
    vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;
    
    CVEWindow& Window;
    vk::raii::PhysicalDevice PhysicalDevice = nullptr;
    uint32_t QueueFamilyIndex = ~0;
    vk::raii::Device LogicalDevice = nullptr;
    vk::raii::Queue GraphicsQueue = nullptr;
    
    vk::raii::SurfaceKHR Surface = nullptr;
    
    vk::raii::CommandPool CommandPool = nullptr;
};
