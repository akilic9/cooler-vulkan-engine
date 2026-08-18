#pragma once
#include <string>
#include <vector>
#include <glm.hpp>

#include "CVETexture.h"

class CVEWindow;
class CVESwapChain;
class CVEDevice;

struct CVEUniformBufferObject
{
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Projection;
};

class CVERenderer
{
public:
    CVERenderer(CVEDevice& inDevice, CVESwapChain& inSwapChain, CVEWindow& inWindow);
    ~CVERenderer();
    
    CVERenderer(const CVERenderer&) = delete;
    CVERenderer& operator=(const CVERenderer&) = delete;
    CVERenderer(CVERenderer&&) = delete;
    CVERenderer& operator=(CVERenderer&&) = delete;
    
    void Draw();
    
private:
    static std::vector<char> ReadShaderFile(const std::string& fileName);
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void CreateShaderModule(const std::vector<char>& shaderCode, VkShaderModule* shaderModule) const;
    
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateCommandBuffers();
    void RecordCommandBuffer(const uint32_t imageIndex);
    void TransitionImageLayout(VkImage               image,
                               VkImageLayout         oldLayout,
                               VkImageLayout         newLayout,
                               VkAccessFlags2        srcAccessMask,
                               VkAccessFlags2        dstAccessMask,
                               VkPipelineStageFlags2 srcStageMask,
                               VkPipelineStageFlags2 dstStageMask,
                               VkImageAspectFlagBits aspectMask);
    
    
    void UpdateUniformBuffer(uint32_t currentFrameIndex);
    void RecreateSwapChain();
    
    CVEDevice& Device;
    CVESwapChain& SwapChain;
    CVEWindow& Window;
    
    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;
    VkDescriptorSetLayout DescriptorSetLayout;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;
    
    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBuffersMemory;
    std::vector<void *> UniformBuffersMapped;
    
    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;
    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;
    std::vector<VkCommandBuffer> CommandBuffers;
    
    uint32_t CurrentFrameIndex = 0;
    
    const std::vector<CVEVertex> Vertices {
        {{-0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};
    
    const std::vector<uint16_t> Indices{0, 1, 2, 2, 3, 0,
                                        4, 5, 6, 6, 7, 4};
    
    CVETexture Texture;
};
