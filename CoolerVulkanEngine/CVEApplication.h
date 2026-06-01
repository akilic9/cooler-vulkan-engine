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
    
    vk::raii::Instance VulkanInstance = nullptr;
    vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;
    GLFWwindow* Window = nullptr;
};
