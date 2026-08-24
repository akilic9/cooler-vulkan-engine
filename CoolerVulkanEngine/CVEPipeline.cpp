#include "CVEPipeline.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include "CVEDevice.h"
#include "CVESwapChain.h"
#include "CVETypes.h"

CVEPipeline::CVEPipeline(CVEDevice& device,
                         const CVESwapChain& swapChain,
                         VkPipelineLayout pipelineLayout)
    : Device(device)
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
    viewport.width    = static_cast<float>(swapChain.GetExtent().width);
    viewport.height   = static_cast<float>(swapChain.GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = VkOffset2D{0, 0};
    scissor.extent = swapChain.GetExtent();
    
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
    
    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.depthTestEnable       = VK_TRUE;
    depthStencilState.depthWriteEnable      = VK_TRUE;
    depthStencilState.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable     = VK_FALSE;
    
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    
    VkFormat colorFormat = swapChain.GetSurfaceFormat().format;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
    
    VkFormat depthFormat = swapChain.GetDepthFormat();
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
    pipelineCreateInfo.layout              = pipelineLayout;
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

CVEPipeline::~CVEPipeline()
{
    vkDestroyPipeline(Device.GetLogicalDevice(), GraphicsPipeline, nullptr);
}

void CVEPipeline::Bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
}

std::vector<char> CVEPipeline::ReadShaderFile(const std::string& fileName)
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

void CVEPipeline::CreateShaderModule(const std::vector<char>& shaderCode, VkShaderModule* shaderModule) const
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
