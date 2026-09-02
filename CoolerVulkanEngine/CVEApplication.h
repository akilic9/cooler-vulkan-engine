#pragma once
#include <memory>

#include "CVEWindow.h"
#include "CVEDevice.h"
#include "CVESwapChain.h"
#include "CVERenderer.h"

class CVEModel;

namespace CVEDefaultWindowParams
{
    static constexpr uint32_t DEFAULT_WINDOW_WIDTH = 1600;
    static constexpr uint32_t DEFAULT_WINDOW_HEIGHT = 900;
    static const char* DEFAULT_WINDOW_TITLE = "CoolerVulkanEngine";
}

class CVEApplication
{
public:
    CVEApplication();
    ~CVEApplication();
    
    CVEApplication(const CVEApplication&) = delete;
    CVEApplication& operator=(const CVEApplication&) = delete;
    CVEApplication(CVEApplication&&) = delete;
    CVEApplication& operator=(CVEApplication&&) = delete;
    
    void Run();

private:
    void CreateDescriptorSetLayout();
    void CreatePipelineLayout();
    void CreateDescriptorPool();
    void CreateDescriptorSets();    
    void CreateSceneUniformBuffers();
    
    void UpdateSceneUBO();
    void Update();
    
    CVEWindow Window{CVEDefaultWindowParams::DEFAULT_WINDOW_WIDTH, CVEDefaultWindowParams::DEFAULT_WINDOW_HEIGHT, CVEDefaultWindowParams::DEFAULT_WINDOW_TITLE};
    CVEDevice Device{Window};
    CVESwapChain SwapChain{Device, Window.GetWindowExtent()};
    CVERenderer Renderer{Device, SwapChain, Window};
    
    std::unique_ptr<CVEModel> Model;
    
    VkDescriptorSetLayout SceneDescriptorSetLayout;
    VkDescriptorSetLayout ModelDescriptorSetLayout;
    
    VkPipelineLayout PipelineLayout;
    std::unique_ptr<CVEPipeline> Pipeline;
    
    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBuffersMemory;
    std::vector<void *> UniformBuffersMapped;
    
    VkDescriptorPool DescriptorPool;
    
    std::vector<VkDescriptorSet> SceneDescriptorSets;
};
