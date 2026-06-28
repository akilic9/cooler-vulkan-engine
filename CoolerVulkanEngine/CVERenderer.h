#pragma once
#include <string>
#include <vector>
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

class CVEWindow;
class CVESwapChain;
class CVEDevice;

class CVERenderer
{
public:
    CVERenderer(CVEDevice& inDevice, CVESwapChain& inSwapChain, CVEWindow& inWindow);
    ~CVERenderer();
    
    CVERenderer(const CVERenderer&) = delete;
    CVERenderer& operator=(const CVERenderer&) = delete;
    CVERenderer(CVERenderer&&) = delete;
    CVERenderer& operator=(CVERenderer&&) = delete;
    
    void RecreateSwapChain();
    
    void Draw();
    
private:
    static std::vector<char> ReadShaderFile(const std::string& fileName);
    void CreateGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& shaderCode) const;
    
    void CreateCommandBuffers();
    void RecordCommandBuffer(const uint32_t imageIndex);
    void TransitionImageLayout(uint32_t                imageIndex,
                               vk::ImageLayout         old_layout,
                               vk::ImageLayout         new_layout,
                               vk::AccessFlags2        src_access_mask,
                               vk::AccessFlags2        dst_access_mask,
                               vk::PipelineStageFlags2 src_stage_mask,
                               vk::PipelineStageFlags2 dst_stage_mask);
    
    CVEDevice& Device;
    CVESwapChain& SwapChain;
    CVEWindow& Window;
    
    vk::raii::PipelineLayout PipelineLayout = nullptr;
    vk::raii::Pipeline GraphicsPipeline = nullptr;
    
    std::vector<vk::raii::CommandBuffer> CommandBuffers;
    
    uint32_t CurrentFrameIndex = 0;
};
