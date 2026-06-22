#include "CVEApplication.h"

#include <fstream>

void CVEApplication::Run()
{
    CreateGraphicsPipeline();
    CommandBuffer = Device.CreateCommandBuffer();
    CreateSyncObjects();
    Update();
    TerminateWindow();
}

void CVEApplication::Update()
{
    while (!Window.GetShouldClose())
    {
        glfwPollEvents();
        DrawFrame();
    }
    Device.CleanUp();
}

void CVEApplication::DrawFrame()
{
    Device.WaitForFences(DrawFence);
    
    vk::ResultValue<uint32_t> acquired = SwapChain.AcquireNextImage(PresentCompleteSemaphore);
    
    RecordCommandBuffer(acquired.value);
    
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    
    const vk::SubmitInfo submitInfo {
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*PresentCompleteSemaphore,
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*CommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*RenderFinishedSemaphore};
    
    Device.SubmitToQueue(DrawFence, submitInfo);
    
    const vk::PresentInfoKHR presentInfoKHR {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*RenderFinishedSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &*SwapChain.GetSwapChain(),
        .pImageIndices      = &acquired.value};
    
    Device.PresentKHR(presentInfoKHR);
}

void CVEApplication::TerminateWindow()
{
    Window.Terminate();
}

// TODO: Pipeline logic should not live here, just not clear where to fit it yet
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

void CVEApplication::RecordCommandBuffer(const uint32_t imageIndex)
{
    CommandBuffer.begin({});
    
    TransitionImageLayout(imageIndex,
                 vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
         {},
         vk::AccessFlagBits2::eColorAttachmentWrite,
          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
          vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

    const std::vector<vk::raii::ImageView>& SwapChainImageViews = SwapChain.GetSwapChainImageViews();

    const vk::Extent2D& SwapChainExtent = SwapChain.GetExtent();
    
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView   = SwapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor};
    
    vk::RenderingInfo renderingInfo = {
        .renderArea           = {
            .offset = {0, 0},
            .extent = SwapChainExtent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo};
    
    CommandBuffer.beginRendering(renderingInfo);
    
    CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *GraphicsPipeline);
    
    const float extentWidth = static_cast<float>(SwapChainExtent.width);
    const float extentHeight = static_cast<float>(SwapChainExtent.height);

    const vk::Viewport viewport = vk::Viewport(0.0f, 0.0f, extentWidth, extentHeight, 0.0f, 1.0f);
    
    CommandBuffer.setViewport(0, viewport);
    
    CommandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), SwapChainExtent));
    
    CommandBuffer.draw(3, 1, 0, 0);
    
    CommandBuffer.endRendering();
    
    TransitionImageLayout(imageIndex,
                 vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::ePresentSrcKHR,
         vk::AccessFlagBits2::eColorAttachmentWrite,
         {},
          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
          vk::PipelineStageFlagBits2::eBottomOfPipe);
    
    CommandBuffer.end();
}

void CVEApplication::TransitionImageLayout(uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                                           vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
                                           vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask)
{
    const std::vector<vk::Image>& swapChainImages = SwapChain.GetSwapChainImages();
    
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = src_stage_mask,
        .srcAccessMask       = src_access_mask,
        .dstStageMask        = dst_stage_mask,
        .dstAccessMask       = dst_access_mask,
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = swapChainImages[imageIndex],
        .subresourceRange    = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1}};
    
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier};
    
    CommandBuffer.pipelineBarrier2(dependencyInfo);
}

void CVEApplication::CreateSyncObjects()
{
    PresentCompleteSemaphore = vk::raii::Semaphore(Device.GetLogicalDevice(), vk::SemaphoreCreateInfo());
    RenderFinishedSemaphore = vk::raii::Semaphore(Device.GetLogicalDevice(), vk::SemaphoreCreateInfo());
    DrawFence = vk::raii::Fence(Device.GetLogicalDevice(), {.flags = vk::FenceCreateFlagBits::eSignaled});
}
