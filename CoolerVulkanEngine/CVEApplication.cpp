#include "CVEApplication.h"

#include <fstream>

void CVEApplication::Run()
{
    CreateGraphicsPipeline();
    Update();
}

void CVEApplication::Update()
{
    while (!Window.GetShouldClose())
    {
        glfwPollEvents();
    }
}

void CVEApplication::TerminateWindow()
{
    Window.Terminate();
}

std::vector<char> CVEApplication::ReadShaderFile(const std::string& fileName)
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

void CVEApplication::CreateGraphicsPipeline()
{
    const std::vector<char>& vertexShader = ReadShaderFile("Shaders/triangle.vert.spv");
    vk::raii::ShaderModule vertexShaderModule = CreateShaderModule(vertexShader);
    
    const std::vector<char>& fragmentShader = ReadShaderFile("Shaders/triangle.frag.spv");
    vk::raii::ShaderModule fragmentShaderModule = CreateShaderModule(fragmentShader);
    
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {
        .stage  = vk::ShaderStageFlagBits::eVertex,
        .module = vertexShaderModule, 
        .pName  = "main"};
    
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {
        .stage  = vk::ShaderStageFlagBits::eFragment,
        .module = fragmentShaderModule, 
        .pName  = "main"};
    
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    const std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data()};
    
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {.topology = vk::PrimitiveTopology::eTriangleList};
    
    vk::Viewport viewport {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = static_cast<float>(SwapChain.GetExtent().width),
        .height   = static_cast<float>(SwapChain.GetExtent().height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f};
    
    vk::Rect2D scissor{vk::Offset2D{ 0, 0 }, SwapChain.GetExtent()};
    
    vk::PipelineViewportStateCreateInfo viewportState {
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor};
    
    vk::PipelineRasterizationStateCreateInfo rasterizer {
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f};
    
    vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
    
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable         = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    
    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable   = vk::False,
        .logicOp         = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachment};
    
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0, .pushConstantRangeCount = 0};

    PipelineLayout = vk::raii::PipelineLayout(Device.GetLogicalDevice(), pipelineLayoutInfo);
    
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {.stageCount              = 2,
            .pStages                 = shaderStages,
            .pVertexInputState       = &vertexInputInfo,
            .pInputAssemblyState     = &inputAssembly,
            .pViewportState          = &viewportState,
            .pRasterizationState     = &rasterizer,
            .pMultisampleState       = &multisampling,
            .pColorBlendState        = &colorBlending,
            .pDynamicState           = &dynamicState,
            .layout                  = PipelineLayout,
            .renderPass              = nullptr},
        {.colorAttachmentCount    = 1,
            .pColorAttachmentFormats = &SwapChain.GetSurfaceFormat().format}};
    
    GraphicsPipeline = vk::raii::Pipeline(Device.GetLogicalDevice(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

vk::raii::ShaderModule CVEApplication::CreateShaderModule(const std::vector<char>& shaderCode) const
{
    const auto codeSize =  shaderCode.size() * sizeof(char);
    const uint32_t* convertedCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
    
    vk::ShaderModuleCreateInfo createInfo{.codeSize = codeSize, .pCode = convertedCode};
    
    vk::raii::ShaderModule shaderModule{Device.GetLogicalDevice(), createInfo};
    
    return shaderModule;
}
