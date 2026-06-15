#pragma once
#include "CVEDevice.h"
#include "CVESwapChain.h"
#include "CVEWindow.h"

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
    
    static std::vector<char> ReadShaderFile(const std::string& fileName);
    void CreateGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& shaderCode) const;
    
    CVEWindow Window{CVEDefaultWindowParams::DEFAULT_WINDOW_WIDTH, CVEDefaultWindowParams::DEFAULT_WINDOW_HEIGHT, CVEDefaultWindowParams::DEFAULT_WINDOW_TITLE};
    CVEDevice Device{Window};
    CVESwapChain SwapChain{Device, Window};
    
    vk::raii::PipelineLayout PipelineLayout = nullptr;
    vk::raii::Pipeline GraphicsPipeline = nullptr;
};
