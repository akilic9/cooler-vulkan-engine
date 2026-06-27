#pragma once
#include "CVEWindow.h"
#include "CVEDevice.h"
#include "CVESwapChain.h"
#include "CVERenderer.h"

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
    void Update();
    void TerminateWindow();
    
    CVEWindow Window{CVEDefaultWindowParams::DEFAULT_WINDOW_WIDTH, CVEDefaultWindowParams::DEFAULT_WINDOW_HEIGHT, CVEDefaultWindowParams::DEFAULT_WINDOW_TITLE};
    CVEDevice Device{Window};
    CVESwapChain SwapChain{Device, Window.GetWindowExtent()};
    CVERenderer Renderer{Device, SwapChain};
};
