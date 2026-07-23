#include "CVESwapChain.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "CVEDevice.h"
#include "CVEWindow.h"

CVESwapChain::CVESwapChain(CVEDevice& device, const std::array<int, 2>& windowExtent)
    : Device(device)
    , Surface(Device.GetSurface())
{
    CreateSwapChain(windowExtent);
    CreateImageViews();
    CreateSyncObjects();
}

CVESwapChain::~CVESwapChain()
{
    for (const VkImageView& imageView : SwapChainImageViews)
    {
        vkDestroyImageView(Device.GetLogicalDevice(), imageView, nullptr);
    }
    SwapChainImageViews.clear();
    
    if (SwapChain != nullptr)
    {
        vkDestroySwapchainKHR(Device.GetLogicalDevice(), SwapChain, nullptr);
        SwapChain = nullptr;
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(Device.GetLogicalDevice(), PresentCompleteSemaphores[i], nullptr);
        vkDestroyFence(Device.GetLogicalDevice(), InFlightFences[i], nullptr);
    }
    
    for (const VkSemaphore& semaphore : RenderFinishedSemaphores)
    {
        vkDestroySemaphore(Device.GetLogicalDevice(), semaphore, nullptr);
    }
}

void CVESwapChain::CreateSwapChain(const std::array<int, 2>& windowExtent)
{
    const CVESwapChainSupportDetails& SwapChainDetails = Device.GetSwapChainSupportDetails();
    uint32_t minImageCount = ChooseMinImageCount(SwapChainDetails.SurfaceCapabilities);
    SurfaceFormat = ChooseSurfaceFormat(SwapChainDetails.AvailableFormats);
    Extent = ChooseExtent(SwapChainDetails.SurfaceCapabilities, windowExtent);
    
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface          = Surface;
    swapChainCreateInfo.minImageCount    = minImageCount;
    swapChainCreateInfo.imageFormat      = SurfaceFormat.format;
    swapChainCreateInfo.imageColorSpace  = SurfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent      = Extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.preTransform     = SwapChainDetails.SurfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode      = ChoosePresentMode(SwapChainDetails.AvailablePresentModes);
    swapChainCreateInfo.clipped          = true;
    
    VkSwapchainKHR oldSwapChain = SwapChain;
    swapChainCreateInfo.oldSwapchain = oldSwapChain;
        
    if (vkCreateSwapchainKHR(Device.GetLogicalDevice(), &swapChainCreateInfo, nullptr, &SwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }
    
    if (oldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(Device.GetLogicalDevice(), oldSwapChain, nullptr);
    }
    
    vkGetSwapchainImagesKHR(Device.GetLogicalDevice(), SwapChain, &minImageCount, nullptr);
    SwapChainImages.resize(minImageCount);
    vkGetSwapchainImagesKHR(Device.GetLogicalDevice(), SwapChain, &minImageCount, SwapChainImages.data());
}

void CVESwapChain::RecreateSwapChain(const std::array<int, 2>& windowExtent)
{
    CleanUp();
    CreateSwapChain(windowExtent);
    CreateImageViews();
}

const VkSurfaceFormatKHR& CVESwapChain::GetSurfaceFormat() const
{
    return SurfaceFormat;
}

const VkSwapchainKHR& CVESwapChain::GetSwapChain() const
{
    return SwapChain;
}

const VkExtent2D& CVESwapChain::GetExtent() const
{
    return Extent;
}

const std::vector<VkImage>& CVESwapChain::GetSwapChainImages() const
{
    return SwapChainImages;
}

const std::vector<VkImageView>& CVESwapChain::GetSwapChainImageViews() const
{
    return SwapChainImageViews;
}

VkResult CVESwapChain::AcquireNextImage(const uint32_t frameIndex, uint32_t* imageIndex)
{
    VkResult result = vkAcquireNextImageKHR(Device.GetLogicalDevice(), SwapChain, UINT64_MAX, PresentCompleteSemaphores[frameIndex], VK_NULL_HANDLE, imageIndex);
    
    return result;
}

void CVESwapChain::WaitForFences(const uint32_t frameIndex)
{    
    const VkResult fenceResult = vkWaitForFences(Device.GetLogicalDevice(), 1, &InFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
    
    if (fenceResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to wait for fence!");
    }
}

void CVESwapChain::ResetFences(const uint32_t frameIndex)
{
    vkResetFences(Device.GetLogicalDevice(), 1, &InFlightFences[frameIndex]);
}

VkResult CVESwapChain::SubmitCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t frameIndex, const uint32_t imageIndex)
{
    VkPipelineStageFlags waitDestinationStageMask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &PresentCompleteSemaphores[frameIndex];
    submitInfo.pWaitDstStageMask    = &waitDestinationStageMask;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &RenderFinishedSemaphores[imageIndex];
    
    if (vkQueueSubmit(Device.GetGraphicsQueue(), 1, &submitInfo, InFlightFences[frameIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfoKHR{};
    presentInfoKHR.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfoKHR.waitSemaphoreCount = 1;
    presentInfoKHR.pWaitSemaphores    = &RenderFinishedSemaphores[imageIndex];
    presentInfoKHR.swapchainCount     = 1;
    presentInfoKHR.pSwapchains        = &SwapChain;
    presentInfoKHR.pImageIndices      = &imageIndex;
    
    return vkQueuePresentKHR(Device.GetGraphicsQueue(), &presentInfoKHR);
}

VkExtent2D CVESwapChain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, std::array<int, 2> windowExtent)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    VkExtent2D extent{};
    extent.width  = std::clamp<uint32_t>(windowExtent[0], capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp<uint32_t>(windowExtent[1], capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    
    return extent;
}

VkSurfaceFormatKHR CVESwapChain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    const auto formatItr = std::ranges::find_if(availableFormats, [](const VkSurfaceFormatKHR& format)
    {
        return format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    
    return formatItr != availableFormats.end() ? *formatItr : availableFormats[0];
}

VkPresentModeKHR CVESwapChain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    const bool bSupportsFIFO = std::ranges::any_of(availablePresentModes, [](const VkPresentModeKHR& presentMode)
    {
        return presentMode == VK_PRESENT_MODE_FIFO_KHR;
    });
    
    assert(bSupportsFIFO);
    
    const bool bSupportsMailbox = std::ranges::any_of(availablePresentModes, [](const VkPresentModeKHR& presentMode)
    {
        return VK_PRESENT_MODE_MAILBOX_KHR == presentMode;
    });

#ifdef _DEBUG
    const char* presentModeText = bSupportsMailbox ? "Mailbox" : "FIFO";
    std::cout << "Present mode: " << presentModeText << '\n';
#endif //_DEBUG
    
    return bSupportsMailbox ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t CVESwapChain::ChooseMinImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
    uint32_t minImageCount = std::max(3u, capabilities.minImageCount);
    if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < minImageCount))
    {
        minImageCount = capabilities.maxImageCount;
    }
    return minImageCount;
}

void CVESwapChain::CreateImageViews()
{
    assert(SwapChainImageViews.empty());
    SwapChainImageViews.resize(SwapChainImages.size());

    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format                      = SurfaceFormat.format;
    imageViewCreateInfo.components.r                = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g                = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b                = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a                = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    
    for (size_t i = 0; i < SwapChainImages.size(); i++)
    {
        imageViewCreateInfo.image = SwapChainImages[i];
        if (vkCreateImageView(Device.GetLogicalDevice(), &imageViewCreateInfo, nullptr, &SwapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture image view!");
        }
    }
}

void CVESwapChain::CreateSyncObjects()
{
    assert(PresentCompleteSemaphores.empty() && RenderFinishedSemaphores.empty() && InFlightFences.empty());
    
    RenderFinishedSemaphores.resize(SwapChainImages.size());
    PresentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    const VkDevice& logicalDevice = Device.GetLogicalDevice();
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    for (size_t i = 0; i < SwapChainImages.size(); i++)
    {
        if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &RenderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &PresentCompleteSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(logicalDevice, &fenceInfo, nullptr, &InFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

void CVESwapChain::CleanUp()
{
    for (const VkImageView& imageView : SwapChainImageViews)
    {
        vkDestroyImageView(Device.GetLogicalDevice(), imageView, nullptr);
    }
    SwapChainImageViews.clear();
}
