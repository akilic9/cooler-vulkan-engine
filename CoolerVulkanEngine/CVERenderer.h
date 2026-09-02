#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

class CVEPipeline;
class CVEWindow;
class CVESwapChain;
class CVEDevice;

class CVERenderer
{
public:
    CVERenderer(CVEDevice& device, CVESwapChain& swapChain, CVEWindow& window);
    ~CVERenderer();
    
    CVERenderer(const CVERenderer&) = delete;
    CVERenderer& operator=(const CVERenderer&) = delete;
    CVERenderer(CVERenderer&&) = delete;
    CVERenderer& operator=(CVERenderer&&) = delete;
    
    uint32_t GetCurrentFrameIndex() const;
    VkCommandBuffer GetCurrentCommandBuffer() const;
    
    VkCommandBuffer BeginDraw();
    void BeginRecordCommandBuffer();    
    void EndDraw();
    
private:
    void CreateCommandBuffers();
    void TransitionImageLayout(VkImage               image,
                               VkImageLayout         oldLayout,
                               VkImageLayout         newLayout,
                               VkAccessFlags2        srcAccessMask,
                               VkAccessFlags2        dstAccessMask,
                               VkPipelineStageFlags2 srcStageMask,
                               VkPipelineStageFlags2 dstStageMask,
                               VkImageAspectFlagBits aspectMask);
    
    void RecreateSwapChain();
    
    CVEDevice& Device;
    CVESwapChain& SwapChain;
    CVEWindow& Window;
    
    std::vector<VkCommandBuffer> CommandBuffers;
    
    uint32_t CurrentFrameIndex = 0;
    uint32_t CurrentImageIndex = 0;
    bool Drawing = false;
};
