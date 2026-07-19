#pragma once
#include <string>
#include <vector>
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <glm.hpp>

class CVEWindow;
class CVESwapChain;
class CVEDevice;

struct CVEVertex
{
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoord0;
    
    static vk::VertexInputBindingDescription GetBindingDesc()
    {
        return {
            .binding = 0,
            .stride = sizeof(CVEVertex),
            .inputRate = vk::VertexInputRate::eVertex};
    }
    
    static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDesc()
    {
        return {{
            {.location = 0,
                .binding  = 0,
                .format   = vk::Format::eR32G32Sfloat,
                .offset   = offsetof(CVEVertex, Position)},
            {.location = 1,
                .binding  = 0,
                .format   = vk::Format::eR32G32B32Sfloat,
                .offset   = offsetof(CVEVertex, Color)},
            {.location = 2,
                .binding  = 0,
                .format   = vk::Format::eR32G32B32Sfloat,
                .offset   = offsetof(CVEVertex, Normal)},
            {.location = 3,
                .binding  = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset   = offsetof(CVEVertex, TexCoord0)},
        }}; 
    }
};

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
    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& shaderCode) const;
    
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags propertyFlags);
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CopyBuffer(vk::raii::Buffer& source, vk::raii::Buffer& destination, vk::DeviceSize size);
    void CreateCommandBuffers();
    void RecordCommandBuffer(const uint32_t imageIndex);
    void TransitionImageLayout(uint32_t                imageIndex,
                               vk::ImageLayout         old_layout,
                               vk::ImageLayout         new_layout,
                               vk::AccessFlags2        src_access_mask,
                               vk::AccessFlags2        dst_access_mask,
                               vk::PipelineStageFlags2 src_stage_mask,
                               vk::PipelineStageFlags2 dst_stage_mask);
    
    
    void UpdateUniformBuffer(uint32_t currentFrameIndex);
    void RecreateSwapChain();
    
    CVEDevice& Device;
    CVESwapChain& SwapChain;
    CVEWindow& Window;
    
    vk::raii::DescriptorPool DescriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> DescriptorSets;
    vk::raii::DescriptorSetLayout DescriptorSetLayout = nullptr;
    vk::raii::PipelineLayout PipelineLayout = nullptr;
    vk::raii::Pipeline GraphicsPipeline = nullptr;
    
    std::vector<vk::raii::Buffer> UniformBuffers;
    std::vector<vk::raii::DeviceMemory> UniformBuffersMemory;
    std::vector<void *> UniformBuffersMapped;
    
    vk::raii::Buffer VertexBuffer = nullptr;
    vk::raii::DeviceMemory VertexBufferMemory = nullptr;
    vk::raii::Buffer IndexBuffer = nullptr;
    vk::raii::DeviceMemory IndexBufferMemory = nullptr;
    std::vector<vk::raii::CommandBuffer> CommandBuffers;
    
    uint32_t CurrentFrameIndex = 0;
    
    const std::vector<CVEVertex> Vertices {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}};
    
    const std::vector<uint16_t> Indices{0, 1, 2, 2, 3, 0};
};
