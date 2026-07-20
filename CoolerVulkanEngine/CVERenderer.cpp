#include "CVERenderer.h"

#include <fstream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <chrono>
#include "CVEDevice.h"
#include "CVESwapChain.h"
#include "CVEWindow.h"


CVERenderer::CVERenderer(CVEDevice& inDevice, CVESwapChain& inSwapChain, CVEWindow& inWindow)
    : Device(inDevice)
    , SwapChain(inSwapChain)
    , Window(inWindow)
{
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
}

CVERenderer::~CVERenderer()
{
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
    
    Device.GetLogicalDevice().waitIdle();
    SwapChain.RecreateSwapChain(Window.GetWindowExtent());
}

void CVERenderer::Draw()
{
    SwapChain.WaitForFences(CurrentFrameIndex);
    
    vk::ResultValue<uint32_t> acquired = SwapChain.AcquireNextImage(CurrentFrameIndex);
    
    if (acquired.result == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateSwapChain();
        return;
    }
    
    if (acquired.result != vk::Result::eSuccess && acquired.result != vk::Result::eSuboptimalKHR)
    {
        assert(acquired.result == vk::Result::eTimeout || acquired.result == vk::Result::eNotReady);
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    SwapChain.ResetFences(CurrentFrameIndex);
    uint32_t imageIndex = acquired.value;
    
    CommandBuffers[CurrentFrameIndex].reset();
    RecordCommandBuffer(imageIndex);
    UpdateUniformBuffer(CurrentFrameIndex);

    const vk::Result& result = SwapChain.SubmitCommandBuffer(CommandBuffers[CurrentFrameIndex], CurrentFrameIndex, imageIndex);
    
    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || Window.GetWasResized())
    {
        Window.ResetResizeFlag();
        RecreateSwapChain();
    }
    else
    {
        assert(result == vk::Result::eSuccess);
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
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex};
    
    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = 1, .pBindings = &uboLayoutBinding};
    
    DescriptorSetLayout = vk::raii::DescriptorSetLayout(Device.GetLogicalDevice(), layoutInfo);
}

void CVERenderer::CreateGraphicsPipeline()
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
    
    vk::PipelineShaderStageCreateInfo shaderStages[] {vertShaderStageInfo, fragShaderStageInfo};
    
    const std::vector<vk::DynamicState> dynamicStates {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data()};

    const vk::VertexInputBindingDescription& bindingDescription = CVEVertex::GetBindingDesc();
    const std::array<vk::VertexInputAttributeDescription, 4>& attributeDescriptions = CVEVertex::GetAttributeDesc();
    
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{.vertexBindingDescriptionCount   = 1,
                                                           .pVertexBindingDescriptions      = &bindingDescription,
                                                           .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
                                                           .pVertexAttributeDescriptions    = attributeDescriptions.data()};
    
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
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f};
    
    vk::PipelineMultisampleStateCreateInfo multisampling {
        .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
    
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable         = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    
    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable   = vk::False,
        .logicOp         = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachment};
    
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &*DescriptorSetLayout,
        .pushConstantRangeCount = 0};

    PipelineLayout = vk::raii::PipelineLayout(Device.GetLogicalDevice(), pipelineLayoutInfo);
    
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain {
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

vk::raii::ShaderModule CVERenderer::CreateShaderModule(const std::vector<char>& shaderCode) const
{
    const auto codeSize =  shaderCode.size() * sizeof(char);
    const uint32_t* convertedCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
    
    vk::ShaderModuleCreateInfo createInfo{.codeSize = codeSize, .pCode = convertedCode};
    
    vk::raii::ShaderModule shaderModule{Device.GetLogicalDevice(), createInfo};
    
    return shaderModule;
}

void CVERenderer::CreateVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(Vertices[0]) * Vertices.size();
    
    vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;    
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> stagingBufferAndMemory =
        Device.CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, propertyFlags);
    
    void* data = stagingBufferAndMemory.second.mapMemory(0, bufferSize);
    memcpy(data, Vertices.data(), bufferSize);
    stagingBufferAndMemory.second.unmapMemory();
    
    vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    std::tie(VertexBuffer, VertexBufferMemory) =
        Device.CreateBuffer(bufferSize, usageFlags, vk::MemoryPropertyFlagBits::eDeviceLocal);

    Device.CopyBuffer(stagingBufferAndMemory.first, VertexBuffer, bufferSize);
}

void CVERenderer::CreateIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(Indices[0]) * Indices.size();
    
    vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> stagingBufferAndMemory =
        Device.CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, propertyFlags);

    void *data = stagingBufferAndMemory.second.mapMemory(0, bufferSize);
    memcpy(data, Indices.data(), bufferSize);
    stagingBufferAndMemory.second.unmapMemory();

    vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    std::tie(IndexBuffer, IndexBufferMemory) =
        Device.CreateBuffer(bufferSize, usageFlags, vk::MemoryPropertyFlagBits::eDeviceLocal);

    Device.CopyBuffer(stagingBufferAndMemory.first, IndexBuffer, bufferSize);
}

void CVERenderer::CreateUniformBuffers()
{
    for (int i = 0; i < CVESwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DeviceSize bufferSize = sizeof(CVEUniformBufferObject);
        vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> uniformBufferAndMemory =
            Device.CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, propertyFlags);
        
        UniformBuffers.emplace_back(std::move(uniformBufferAndMemory.first));
        UniformBuffersMemory.emplace_back(std::move(uniformBufferAndMemory.second));
        UniformBuffersMapped.emplace_back(UniformBuffersMemory.back().mapMemory(0, bufferSize));
    }
}

void CVERenderer::CreateDescriptorPool()
{
    vk::DescriptorPoolSize poolSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = CVESwapChain::MAX_FRAMES_IN_FLIGHT};
    
    vk::DescriptorPoolCreateInfo poolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = CVESwapChain::MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize};
    
    DescriptorPool = vk::raii::DescriptorPool(Device.GetLogicalDevice(), poolInfo);
}

void CVERenderer::CreateDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(CVESwapChain::MAX_FRAMES_IN_FLIGHT, *DescriptorSetLayout);
    
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool     = DescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()};
    
    DescriptorSets = Device.GetLogicalDevice().allocateDescriptorSets(allocInfo);
    
    for (int i = 0; i < CVESwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo {
            .buffer = UniformBuffers[i],
            .offset = 0,
            .range = sizeof(CVEUniformBufferObject)};
        
        vk::WriteDescriptorSet descriptorWrite {
            .dstSet          = DescriptorSets[i],
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo     = &bufferInfo};
        
        Device.GetLogicalDevice().updateDescriptorSets(descriptorWrite, {});
    }
}

void CVERenderer::CreateCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocateInfo { 
        .commandPool        = Device.GetCommandPool(),
        .level              = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = CVESwapChain::MAX_FRAMES_IN_FLIGHT};

    CommandBuffers = vk::raii::CommandBuffers(Device.GetLogicalDevice(), allocateInfo);
}

void CVERenderer::RecordCommandBuffer(const uint32_t imageIndex)
{
    vk::raii::CommandBuffer& CurrentCommandBuffer = CommandBuffers[CurrentFrameIndex];
    
    CurrentCommandBuffer.begin({});
    
    TransitionImageLayout(imageIndex,
                 vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
         {},
         vk::AccessFlagBits2::eColorAttachmentWrite,
          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
          vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

    const std::vector<vk::raii::ImageView>& SwapChainImageViews = SwapChain.GetSwapChainImageViews();
    
    vk::RenderingAttachmentInfo attachmentInfo {
        .imageView   = SwapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor};
    
    const vk::Extent2D& SwapChainExtent = SwapChain.GetExtent();
    
    vk::RenderingInfo renderingInfo {
        .renderArea{
            .offset = {0, 0},
            .extent = SwapChainExtent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo};
    
    CurrentCommandBuffer.beginRendering(renderingInfo);
    
    CurrentCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *GraphicsPipeline);
    CurrentCommandBuffer.bindVertexBuffers(0, *VertexBuffer, {0});
    CurrentCommandBuffer.bindIndexBuffer(*IndexBuffer, 0, vk::IndexType::eUint16);
    CurrentCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, PipelineLayout, 0, *DescriptorSets[CurrentFrameIndex], nullptr);
    
    const float extentWidth = static_cast<float>(SwapChainExtent.width);
    const float extentHeight = static_cast<float>(SwapChainExtent.height);

    const vk::Viewport viewport = vk::Viewport(0.0f, 0.0f, extentWidth, extentHeight, 0.0f, 1.0f);
    
    CurrentCommandBuffer.setViewport(0, viewport);
    
    CurrentCommandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), SwapChainExtent));
    
    CurrentCommandBuffer.drawIndexed(static_cast<uint32_t>(Indices.size()), 1, 0, 0, 0);
    
    CurrentCommandBuffer.endRendering();
    
    TransitionImageLayout(imageIndex,
                 vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::ePresentSrcKHR,
         vk::AccessFlagBits2::eColorAttachmentWrite,
         {},
          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
          vk::PipelineStageFlagBits2::eBottomOfPipe);
    
    CurrentCommandBuffer.end();
}

void CVERenderer::TransitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                        vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                        vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask)
{
    const std::vector<vk::Image>& swapChainImages = SwapChain.GetSwapChainImages();
    
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = srcStageMask,
        .srcAccessMask       = srcAccessMask,
        .dstStageMask        = dstStageMask,
        .dstAccessMask       = dstAccessMask,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
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
    
    CommandBuffers[CurrentFrameIndex].pipelineBarrier2(dependencyInfo);
}
