#include "CVERenderer.h"

#include <fstream>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>

#include "CVEDevice.h"
#include "CVESwapChain.h"
#include "CVETypes.h"
#include "CVEWindow.h"

// TODO : This file need cleanup, spit some stuff to their own classes, some functions can be simplified

CVERenderer::CVERenderer(CVEDevice& inDevice, CVESwapChain& inSwapChain, CVEWindow& inWindow)
    : Device(inDevice)
    , SwapChain(inSwapChain)
    , Window(inWindow)
{
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
}

CVERenderer::~CVERenderer()
{
    vkDestroyPipeline(Device.GetLogicalDevice(), GraphicsPipeline, nullptr);
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
    
    vkFreeCommandBuffers(Device.GetLogicalDevice(), Device.GetCommandPool(),
        static_cast<uint32_t>(CommandBuffers.size()), CommandBuffers.data());
    CommandBuffers.clear();
}

void CVERenderer::RecreateSwapChain()
{
    std::array<int, 2> windowExtent = Window.GetWindowExtent();
    while (windowExtent[0] == 0 || windowExtent[1] == 0)
    {
        // pause when minimized
        windowExtent = Window.GetWindowExtent();
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(Device.GetLogicalDevice());
    SwapChain.RecreateSwapChain(Window.GetWindowExtent());
}

void CVERenderer::Draw()
{
    SwapChain.WaitForFences(CurrentFrameIndex);
    
    uint32_t imageIndex = 0;
    VkResult result = SwapChain.AcquireNextImage(CurrentFrameIndex, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }
    
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        assert(result == VK_TIMEOUT || result == VK_NOT_READY);
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    UpdateUniformBuffer(CurrentFrameIndex);
    
    SwapChain.ResetFences(CurrentFrameIndex);
    
    vkResetCommandBuffer(CommandBuffers[CurrentFrameIndex], 0);
    RecordCommandBuffer(imageIndex);

    result = SwapChain.SubmitCommandBuffer(CommandBuffers[CurrentFrameIndex], CurrentFrameIndex, imageIndex);
    
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || Window.GetWasResized())
    {
        Window.ResetResizeFlag();
        RecreateSwapChain();
    }
    else
    {
        assert(result == VK_SUCCESS);
    }
    
    CurrentFrameIndex = (CurrentFrameIndex + 1) % CVESwapChain::MAX_FRAMES_IN_FLIGHT;
}

void CVERenderer::UpdateUniformBuffer(uint32_t currentFrameIndex)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    
    CVEUniformBufferObject ubo{};
    ubo.Model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.View = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.Projection = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(SwapChain.GetExtent().width) / static_cast<float>(SwapChain.GetExtent().height),
                                0.1f,
                                10.0f);
    
    ubo.Projection[1][1] *= -1; // Flip Y to compensate glm coord system
    
    memcpy(UniformBuffersMapped[currentFrameIndex], &ubo, sizeof(ubo));
}

std::vector<char> CVERenderer::ReadShaderFile(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + fileName);
    }
    
    std::vector<char> fileBuffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(fileBuffer.data(), static_cast<std::streamsize>(fileBuffer.size()));
    
    file.close();

    return fileBuffer;
}

void CVERenderer::CreateDescriptorSetLayout()
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

// TODO: Split/move
void CVERenderer::CreateGraphicsPipeline()
{
    const std::vector<char>& vertexShader = ReadShaderFile("Shaders/triangle.vert.spv");
    VkShaderModule vertexShaderModule;
    CreateShaderModule(vertexShader, &vertexShaderModule);
    
    const std::vector<char>& fragmentShader = ReadShaderFile("Shaders/triangle.frag.spv");
    VkShaderModule fragmentShaderModule;
    CreateShaderModule(fragmentShader, &fragmentShaderModule);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage                = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module               = vertexShaderModule;
    vertShaderStageInfo.pName                = "main";
    vertShaderStageInfo.flags                = 0;
    vertShaderStageInfo.pNext                = nullptr;
    vertShaderStageInfo.pSpecializationInfo  = nullptr;
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage                = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module               = fragmentShaderModule;
    fragShaderStageInfo.pName                = "main";
    fragShaderStageInfo.flags                = 0;
    fragShaderStageInfo.pNext                = nullptr;
    fragShaderStageInfo.pSpecializationInfo  = nullptr;
    
    VkPipelineShaderStageCreateInfo shaderStages[] {vertShaderStageInfo, fragShaderStageInfo};
    
    const std::vector<VkDynamicState> dynamicStates {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    const VkVertexInputBindingDescription& bindingDescription = CVEVertex::GetBindingDesc();
    const std::array<VkVertexInputAttributeDescription, 4>& attributeDescriptions = CVEVertex::GetAttributeDesc();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(SwapChain.GetExtent().width);
    viewport.height   = static_cast<float>(SwapChain.GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = VkOffset2D{0, 0};
    scissor.extent = SwapChain.GetExtent();
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.lineWidth               = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable  = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    
    if (vkCreatePipelineLayout(Device.GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    
    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.depthTestEnable       = VK_TRUE;
    depthStencilState.depthWriteEnable      = VK_TRUE;
    depthStencilState.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable     = VK_FALSE;
    
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    
    VkFormat colorFormat = SwapChain.GetSurfaceFormat().format;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
    
    VkFormat depthFormat = SwapChain.GetDepthFormat();
    pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext               = &pipelineRenderingCreateInfo;
    pipelineCreateInfo.stageCount          = 2;
    pipelineCreateInfo.pStages             = shaderStages;
    pipelineCreateInfo.pVertexInputState   = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState      = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState   = &multisampling;
    pipelineCreateInfo.pColorBlendState    = &colorBlending;
    pipelineCreateInfo.pDynamicState       = &dynamicState;
    pipelineCreateInfo.layout              = PipelineLayout;
    pipelineCreateInfo.renderPass          = VK_NULL_HANDLE;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
    
    if (vkCreateGraphicsPipelines(Device.GetLogicalDevice(), VK_NULL_HANDLE, 1,
        &pipelineCreateInfo, nullptr, &GraphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
    
    vkDestroyShaderModule(Device.GetLogicalDevice(), fragmentShaderModule, nullptr);
    vkDestroyShaderModule(Device.GetLogicalDevice(), vertexShaderModule, nullptr);
}

void CVERenderer::CreateShaderModule(const std::vector<char>& shaderCode, VkShaderModule* shaderModule) const
{
    const auto codeSize =  shaderCode.size() * sizeof(char);
    const uint32_t* convertedCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = convertedCode;
    
    if (vkCreateShaderModule(Device.GetLogicalDevice(), &createInfo, nullptr, shaderModule) != VK_SUCCESS)
    {
        std::cout << "Failed to create shader module!" << std::endl;
    }
}

void CVERenderer::CreateUniformBuffers()
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

void CVERenderer::CreateDescriptorPool()
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

void CVERenderer::CreateDescriptorSets()
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

void CVERenderer::CreateCommandBuffers()
{
    CommandBuffers.resize(CVESwapChain::MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = Device.GetCommandPool();
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = CVESwapChain::MAX_FRAMES_IN_FLIGHT;
    
    if (vkAllocateCommandBuffers(Device.GetLogicalDevice(), &allocateInfo, CommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void CVERenderer::RecordCommandBuffer(const uint32_t imageIndex)
{
    VkCommandBuffer CurrentCommandBuffer = CommandBuffers[CurrentFrameIndex];
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult beginResult = vkBeginCommandBuffer(CurrentCommandBuffer, &beginInfo);
    if (beginResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin command buffer record!");
    }
    
    TransitionImageLayout(SwapChain.GetSwapChainImages()[imageIndex],
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          0,
                          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT);
    
    TransitionImageLayout(SwapChain.GetDepthImage(),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                          VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                          VK_IMAGE_ASPECT_DEPTH_BIT);
    
    VkClearValue clearColor{};
    clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    const std::vector<VkImageView>& SwapChainImageViews = SwapChain.GetSwapChainImageViews();
    
    VkRenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.pNext       = nullptr;
    attachmentInfo.imageView   = SwapChainImageViews[imageIndex];
    attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue  = clearColor;
    
    VkRenderingAttachmentInfo depthAttachmentInfo{};
    SwapChain.GetDepthAttachmentInfo(depthAttachmentInfo);
    
    const VkExtent2D& SwapChainExtent = SwapChain.GetExtent();
    
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset    = VkOffset2D{0, 0};
    renderingInfo.renderArea.extent    = SwapChainExtent;
    renderingInfo.layerCount           = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments    = &attachmentInfo;
    renderingInfo.pDepthAttachment     = &depthAttachmentInfo;
    
    vkCmdBeginRendering(CurrentCommandBuffer, &renderingInfo);
    vkCmdBindPipeline(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
    
    VkDeviceSize offsets[] = {0};
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
    
    const float extentWidth = static_cast<float>(SwapChainExtent.width);
    const float extentHeight = static_cast<float>(SwapChainExtent.height);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = extentWidth;
    viewport.height   = extentHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(CurrentCommandBuffer, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = SwapChainExtent;

    vkCmdSetScissor(CurrentCommandBuffer, 0, 1, &scissor);

    vkCmdDrawIndexed(CurrentCommandBuffer, static_cast<uint32_t>(Indices.size()), 1, 0, 0, 0);

    vkCmdEndRendering(CurrentCommandBuffer);

    TransitionImageLayout(SwapChain.GetSwapChainImages()[imageIndex],
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                          0,
                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                          VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT);
    
    if (vkEndCommandBuffer(CurrentCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Error command buffer record end!");
    }
}

void CVERenderer::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                        VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask,
                                        VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask,
                                        VkImageAspectFlagBits aspectMask)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask                    = srcStageMask;
    barrier.srcAccessMask                   = srcAccessMask;
    barrier.dstStageMask                    = dstStageMask;
    barrier.dstAccessMask                   = dstAccessMask;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = aspectMask;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    
    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.dependencyFlags         = 0;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers    = &barrier;
    
    vkCmdPipelineBarrier2(CommandBuffers[CurrentFrameIndex], &dependencyInfo);
}
