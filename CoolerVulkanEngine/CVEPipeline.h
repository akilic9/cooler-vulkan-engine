#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

class CVESwapChain;
class CVEDevice;

class CVEPipeline
{
public:
    CVEPipeline(CVEDevice &device,
                const CVESwapChain& swapChain,
                VkPipelineLayout pipelineLayout);
    
    ~CVEPipeline();
    
    CVEPipeline(const CVEPipeline&) = delete;
    CVEPipeline& operator=(const CVEPipeline&) = delete;
    CVEPipeline(CVEPipeline&&) = delete;
    CVEPipeline& operator=(CVEPipeline&&) = delete;
    
    void Bind(VkCommandBuffer commandBuffer);
    
private:
    static std::vector<char> ReadShaderFile(const std::string& fileName);
    void CreateShaderModule(const std::vector<char>& shaderCode, VkShaderModule* shaderModule) const;
        
    CVEDevice& Device;
    VkPipeline GraphicsPipeline;
};
