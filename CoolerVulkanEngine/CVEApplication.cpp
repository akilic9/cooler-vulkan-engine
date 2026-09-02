#include "CVEApplication.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <stdexcept>
#include <gtc/matrix_transform.hpp>

#include "CVEModel.h"
#include "CVEPipeline.h"
#include "CVETypes.h"

CVEApplication::CVEApplication()
{
    Model = std::make_unique<CVEModel>(Device, "Assets/damaged-helmet.fbx");
    CreateDescriptorSetLayout();
    CreatePipelineLayout();
    Pipeline = std::make_unique<CVEPipeline>(Device, SwapChain, PipelineLayout);
    CreateSceneUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
}

CVEApplication::~CVEApplication()
{
    vkDestroyPipelineLayout(Device.GetLogicalDevice(), PipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(Device.GetLogicalDevice(), SceneDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(Device.GetLogicalDevice(), ModelDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(Device.GetLogicalDevice(), DescriptorPool, nullptr);    
    SceneDescriptorSets.clear();
    
    for (size_t i = 0; i < UniformBuffers.size(); i++)
    {
        vkDestroyBuffer(Device.GetLogicalDevice(), UniformBuffers[i], nullptr);
        vkFreeMemory(Device.GetLogicalDevice(), UniformBuffersMemory[i], nullptr);
        UniformBuffersMapped[i] = nullptr;
    }
    
    UniformBuffers.clear();
    UniformBuffersMemory.clear();
    UniformBuffersMapped.clear();
}

void CVEApplication::Run()
{
    while (!Window.GetShouldClose())
    {
        glfwPollEvents();
        if (VkCommandBuffer commandBuffer = Renderer.BeginDraw())
        {
            Update();
            
            Renderer.BeginRecordCommandBuffer();
            
            Pipeline->Bind(commandBuffer);
            Model->Draw(commandBuffer, PipelineLayout);
            
            Renderer.EndDraw();
        }
    }
    vkDeviceWaitIdle(Device.GetLogicalDevice());
}

void CVEApplication::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding         = 0;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &layoutBinding;
    
    if (vkCreateDescriptorSetLayout(Device.GetLogicalDevice(), &layoutInfo, nullptr, &SceneDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
    
    layoutBinding.binding         = 0;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    if (vkCreateDescriptorSetLayout(Device.GetLogicalDevice(), &layoutInfo, nullptr, &ModelDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void CVEApplication::CreatePipelineLayout()
{
    std::array<VkDescriptorSetLayout, 2> setLayouts = { SceneDescriptorSetLayout, ModelDescriptorSetLayout };
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts            = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    
    if (vkCreatePipelineLayout(Device.GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void CVEApplication::CreateDescriptorPool()
{
    const uint32_t textureCount = Model->GetTextureCount();
    
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = CVESwapChain::MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = textureCount * CVESwapChain::MAX_FRAMES_IN_FLIGHT;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = (textureCount + 1) * CVESwapChain::MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    
    if (vkCreateDescriptorPool(Device.GetLogicalDevice(), &poolInfo, nullptr, &DescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool.");
    }
}

void CVEApplication::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> sceneLayouts(CVESwapChain::MAX_FRAMES_IN_FLIGHT, SceneDescriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(sceneLayouts.size());
    allocInfo.pSetLayouts        = sceneLayouts.data();
    
    if (vkAllocateDescriptorSets(Device.GetLogicalDevice(), &allocInfo, SceneDescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set.");
    }
    
    for (int i = 0; i < UniformBuffers.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = UniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CVEUniformBuffer);
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = SceneDescriptorSets[i];
        descriptorWrite.dstBinding      = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.pBufferInfo     = &bufferInfo;
        
        vkUpdateDescriptorSets(Device.GetLogicalDevice(), 1,
            &descriptorWrite, 0, nullptr);
    }
    
    Model->CreateTextureDescriptorSets(DescriptorPool, ModelDescriptorSetLayout);
}

void CVEApplication::CreateSceneUniformBuffers()
{
    for (int i = 0; i < CVESwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDeviceSize bufferSize = sizeof(CVEUniformBuffer);
        VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
        
        Device.CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, propertyFlags, buffer, bufferMemory);
        
        UniformBuffers.emplace_back(buffer);
        UniformBuffersMemory.emplace_back(bufferMemory);
        
        void* data;
        vkMapMemory(Device.GetLogicalDevice(), UniformBuffersMemory.back(), 0, bufferSize, 0, &data);
        UniformBuffersMapped.emplace_back(data);
    }
}

void CVEApplication::UpdateSceneUBO()
{
    CVEUniformBuffer ubo{};
    ubo.View = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.Projection = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(SwapChain.GetExtent().width) / static_cast<float>(SwapChain.GetExtent().height),
                                0.1f,
                                10.0f);    
    ubo.Projection[1][1] *= -1; // Flip Y to compensate glm coord system
    memcpy(UniformBuffersMapped[Renderer.GetCurrentFrameIndex()], &ubo, sizeof(ubo));
}

void CVEApplication::Update()
{    
    UpdateSceneUBO();
    Model->Update();
}
