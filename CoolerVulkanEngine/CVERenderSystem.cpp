#include "CVERenderSystem.h"

#include <stdexcept>


#include "CVEDevice.h"
#include "CVEPipeline.h"
#include "CVESwapChain.h"
#include "CVETypes.h"

CVERenderSystem::CVERenderSystem(CVEDevice& device, CVESwapChain& swapChain)
    : Device(device)
    , SwapChain(swapChain)
{
    Init();
}

CVERenderSystem::~CVERenderSystem()
{
    vkDestroyPipelineLayout(Device.GetLogicalDevice(), PipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(Device.GetLogicalDevice(), DescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(Device.GetLogicalDevice(), DescriptorPool, nullptr);    
    DescriptorSets.clear();

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

void CVERenderSystem::Init()
{
    CreatePipelineLayout();
    CreateDescriptorSetLayout();
    Pipeline = std::make_unique<CVEPipeline>(Device, SwapChain, PipelineLayout);
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
}

void CVERenderSystem::Update(uint32_t currentFrameIndex, const CVEUniformBufferObject& ubo)
{
    memcpy(UniformBuffersMapped[currentFrameIndex], &ubo, sizeof(ubo));
}

void CVERenderSystem::Render(VkCommandBuffer commandBuffer)
{
    Pipeline->Bind(commandBuffer);
    
    /*VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(CurrentCommandBuffer, 0, 1, &VertexBuffer, offsets);

    vkCmdBindIndexBuffer(CurrentCommandBuffer, IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(CurrentCommandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            PipelineLayout,
                            0,
                            1, 
                            &DescriptorSets[CurrentFrameIndex], 
                            0,
                            nullptr);

    vkCmdDrawIndexed(CurrentCommandBuffer, static_cast<uint32_t>(Indices.size()), 1, 0, 0, 0);*/
}

void CVERenderSystem::CreateDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayout> layouts(CVESwapChain::MAX_FRAMES_IN_FLIGHT, DescriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts        = layouts.data();
    
    DescriptorSets.resize(layouts.size());
    
    if (vkAllocateDescriptorSets(Device.GetLogicalDevice(), &allocInfo, DescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set.");
    }
    
    for (int i = 0; i < CVESwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = UniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CVEUniformBufferObject);
        
        VkDescriptorImageInfo imageInfo = Texture.GetDescriptorImageInfo();
        
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = DescriptorSets[i];
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].pBufferInfo     = &bufferInfo;
        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = DescriptorSets[i];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].pImageInfo      = &imageInfo;
        
        vkUpdateDescriptorSets(Device.GetLogicalDevice(), descriptorWrites.size(),
            descriptorWrites.data(), 0, nullptr);
    }
}

void CVERenderSystem::CreatePipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    
    if (vkCreatePipelineLayout(Device.GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void CVERenderSystem::CreateUniformBuffers()
{
    for (int i = 0; i < CVESwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDeviceSize bufferSize = sizeof(CVEUniformBufferObject);
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

void CVERenderSystem::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = CVESwapChain::MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = CVESwapChain::MAX_FRAMES_IN_FLIGHT;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = CVESwapChain::MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    
    if (vkCreateDescriptorPool(Device.GetLogicalDevice(), &poolInfo, nullptr, &DescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool.");
    }
}

void CVERenderSystem::CreateDescriptorSets()
{
    std::array<VkDescriptorSetLayoutBinding, 2> uboLayoutBindings{};
    uboLayoutBindings[0].binding         = 0;
    uboLayoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBindings[0].descriptorCount = 1;
    uboLayoutBindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBindings[1].binding         = 1;
    uboLayoutBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uboLayoutBindings[1].descriptorCount = 1;
    uboLayoutBindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(uboLayoutBindings.size());
    layoutInfo.pBindings    = uboLayoutBindings.data();
    
    if (vkCreateDescriptorSetLayout(Device.GetLogicalDevice(), &layoutInfo, nullptr, &DescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}
