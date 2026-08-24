#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

struct CVEUniformBufferObject;
class CVESwapChain;
class CVEPipeline;
class CVEDevice;

class CVERenderSystem
{
public:
    CVERenderSystem(CVEDevice& device, CVESwapChain& swapChain);
    ~CVERenderSystem();
    
    CVERenderSystem(const CVERenderSystem&) = delete;
    CVERenderSystem& operator=(const CVERenderSystem&) = delete;
    CVERenderSystem(CVERenderSystem&&) = delete;
    CVERenderSystem& operator=(CVERenderSystem&&) = delete;
    
    void Init();
    void Update(uint32_t currentFrameIndex, const CVEUniformBufferObject& ubo);
    void Render(VkCommandBuffer commandBuffer);
    
private:
    void CreateDescriptorSetLayout();
    void CreatePipelineLayout();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    
    CVEDevice& Device;
    CVESwapChain& SwapChain;
    
    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;
    VkDescriptorSetLayout DescriptorSetLayout;
    VkPipelineLayout PipelineLayout;
    std::unique_ptr<CVEPipeline> Pipeline;
    
    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBuffersMemory;
    std::vector<void *> UniformBuffersMapped;
};
