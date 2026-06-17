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
    
    // TODO: Pipeline logic should not live here, just not clear where to fit it yet
    static std::vector<char> ReadShaderFile(const std::string& fileName);
    void CreateGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& shaderCode) const;
    
    void CreateCommandBuffer();
    void RecordCommandBuffer(const uint32_t imageIndex);
    void TransitionImageLayout(uint32_t                imageIndex,
                               vk::ImageLayout         old_layout,
                               vk::ImageLayout         new_layout,
                               vk::AccessFlags2        src_access_mask,
                               vk::AccessFlags2        dst_access_mask,
                               vk::PipelineStageFlags2 src_stage_mask,
                               vk::PipelineStageFlags2 dst_stage_mask);
    
    CVEWindow Window{CVEDefaultWindowParams::DEFAULT_WINDOW_WIDTH, CVEDefaultWindowParams::DEFAULT_WINDOW_HEIGHT, CVEDefaultWindowParams::DEFAULT_WINDOW_TITLE};
    CVEDevice Device{Window};
    CVESwapChain SwapChain{Device, Window};
    
    // TODO: Pipeline logic should not live here, just not clear where to fit it yet
    vk::raii::PipelineLayout PipelineLayout = nullptr;
    vk::raii::Pipeline GraphicsPipeline = nullptr;
    
    vk::raii::CommandBuffer CommandBuffer = nullptr;
};
