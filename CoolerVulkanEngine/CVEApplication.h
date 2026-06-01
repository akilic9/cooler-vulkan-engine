#pragma once

#include <glfw3.h>
#include "CVEVulkanInstance.h"

namespace CVEDefaultWindowParams
{
    static constexpr uint32_t DEFAULT_WINDOW_WIDTH = 1600;
    static constexpr uint32_t DEFAULT_WINDOW_HEIGHT = 900;
    static const char* DEFAULT_WINDOW_TITLE = "CoolerVulkanEngine";
}

class CVEApplication
{
public:
    void Run();

private:
    void InitApp();
    void InitWindow();
    void Update();
    void TerminateWindow();
        
    void InitPhysicalDevice();
    void InitLogicalDevice();
    bool IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);
    
    std::vector<const char*> RequiredDeviceExtensions = {vk::KHRSwapchainExtensionName};
    
    vk::raii::Instance VulkanInstance = nullptr;
    vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;
    vk::raii::PhysicalDevice PhysicalDevice = nullptr;
    vk::raii::Device LogicalDevice = nullptr;
    vk::raii::Queue GraphicsQueue = nullptr;
    GLFWwindow* Window = nullptr;
};
