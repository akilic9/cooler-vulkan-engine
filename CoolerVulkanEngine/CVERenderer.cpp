#include "CVERenderer.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <chrono>

#include "CVEDevice.h"
#include "CVEPipeline.h"
#include "CVESwapChain.h"
#include "CVEWindow.h"

CVERenderer::CVERenderer(CVEDevice& device, CVESwapChain& swapChain, CVEWindow& window)
    : Device(device)
    , SwapChain(swapChain)
    , Window(window)
{
    CreateCommandBuffers();
}

CVERenderer::~CVERenderer()
{
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

uint32_t CVERenderer::GetCurrentFrameIndex() const
{
    assert(Drawing && "Cannot get frame index when frame not in progress.");
    return CurrentFrameIndex;
}

VkCommandBuffer CVERenderer::GetCurrentCommandBuffer() const
{
    assert(Drawing && "Cannot get current command buffer when frame not in progress.");
    return CommandBuffers[CurrentFrameIndex];
}

VkCommandBuffer CVERenderer::BeginDraw()
{
    assert(!Drawing && "Can't call BeginDraw while draw already in progress.");
    SwapChain.WaitForFences(CurrentFrameIndex);
    
    VkResult result = SwapChain.AcquireNextImage(CurrentFrameIndex, &CurrentImageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return nullptr;
    }
    
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        assert(result == VK_TIMEOUT || result == VK_NOT_READY);
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    SwapChain.ResetFences(CurrentFrameIndex);
    return CommandBuffers[CurrentFrameIndex];
}

void CVERenderer::EndDraw()
{
    VkCommandBuffer CurrentCommandBuffer = CommandBuffers[CurrentFrameIndex];
    vkCmdEndRendering(CurrentCommandBuffer);

    TransitionImageLayout(SwapChain.GetSwapChainImages()[CurrentImageIndex],
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
    
    assert(Drawing && "Can't call EndDraw while frame is not in progress.");
    VkResult result = SwapChain.SubmitCommandBuffer(CommandBuffers[CurrentFrameIndex], CurrentFrameIndex, CurrentImageIndex);
    
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || Window.GetWasResized())
    {
        Window.ResetResizeFlag();
        RecreateSwapChain();
    }
    else
    {
        assert(result == VK_SUCCESS);
    }
    
    Drawing = false;
    CurrentFrameIndex = (CurrentFrameIndex + 1) % CVESwapChain::MAX_FRAMES_IN_FLIGHT;
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

void CVERenderer::BeginRecordCommandBuffer()
{
    VkCommandBuffer CurrentCommandBuffer = CommandBuffers[CurrentFrameIndex];
    
    vkResetCommandBuffer(CurrentCommandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkResult beginResult = vkBeginCommandBuffer(CurrentCommandBuffer, &beginInfo);
    if (beginResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin command buffer record!");
    }
    
    TransitionImageLayout(SwapChain.GetSwapChainImages()[CurrentImageIndex],
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
    attachmentInfo.imageView   = SwapChainImageViews[CurrentImageIndex];
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
