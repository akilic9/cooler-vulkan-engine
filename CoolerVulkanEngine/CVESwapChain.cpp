#include "CVESwapChain.h"

#include <iostream>

#include "CVEDevice.h"
#include "CVEWindow.h"

CVESwapChain::CVESwapChain(const CVEDevice& device, const CVEWindow& window)
    : Device(device)
    , Surface(Device.GetSurface())
{
    const CVESwapChainSupportDetails& SwapChainDetails = Device.GetSwapChainSupportDetails();
    uint32_t minImageCount = ChooseMinImageCount(SwapChainDetails.SurfaceCapabilities);
    SurfaceFormat = ChooseSurfaceFormat(SwapChainDetails.AvailableFormats);
    Extent = ChooseExtent(SwapChainDetails.SurfaceCapabilities, window.GetWindowExtent());
    
    vk::SwapchainCreateInfoKHR swapChainCreateInfo {
        .surface          = *Surface,
        .minImageCount    = minImageCount,
        .imageFormat      = SurfaceFormat.format,
        .imageColorSpace  = SurfaceFormat.colorSpace,
        .imageExtent      = Extent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = SwapChainDetails.SurfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = ChoosePresentMode(SwapChainDetails.AvailablePresentModes),
        .clipped          = true};
    
    swapChainCreateInfo.oldSwapchain = nullptr;
    
    SwapChain = vk::raii::SwapchainKHR(Device.GetLogicalDevice(), swapChainCreateInfo);
    SwapChainImages = SwapChain.getImages();
    
    CreateImageViews();
}

const vk::SurfaceFormatKHR& CVESwapChain::GetSurfaceFormat() const
{
    return SurfaceFormat;
}

const vk::raii::SwapchainKHR& CVESwapChain::GetSwapChain() const
{
    return SwapChain;
}

const vk::Extent2D& CVESwapChain::GetExtent() const
{
    return Extent;
}

const std::vector<vk::Image>& CVESwapChain::GetSwapChainImages() const
{
    return SwapChainImages;
}

const std::vector<vk::raii::ImageView>& CVESwapChain::GetSwapChainImageViews() const
{
    return SwapChainImageViews;
}

vk::ResultValue<uint32_t> CVESwapChain::AcquireNextImage(const vk::raii::Semaphore& presentCompleteSemaphore)
{
    return SwapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore, nullptr);
}

vk::Extent2D CVESwapChain::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const std::array<int, 2>& windowExtent)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    return vk::Extent2D {
        .width = std::clamp<uint32_t>(windowExtent[0], capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp<uint32_t>(windowExtent[1], capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

vk::SurfaceFormatKHR CVESwapChain::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    const auto formatItr = std::ranges::find_if(availableFormats, [](const vk::SurfaceFormatKHR& format)
    {
        return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    
    return formatItr != availableFormats.end() ? *formatItr : availableFormats[0];
}

vk::PresentModeKHR CVESwapChain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    const bool bSupportsFIFO = std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR& presentMode)
    {
        return presentMode == vk::PresentModeKHR::eFifo;
    });
    
    assert(bSupportsFIFO);
    
    const bool bSupportsMailbox = std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR& presentMode)
    {
        return vk::PresentModeKHR::eMailbox == presentMode;
    });

#ifdef _DEBUG
    const char* presentModeText = bSupportsMailbox ? "Mailbox" : "FIFO";
    std::cout << "Present mode: " << presentModeText << '\n';
#endif //_DEBUG
    
    return bSupportsMailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

uint32_t CVESwapChain::ChooseMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities)
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

    vk::ImageViewCreateInfo imageViewCreateInfo {
        .viewType   = vk::ImageViewType::e2D,
        .format     = SurfaceFormat.format,
        .components = {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity},
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1}};
    
    for (const vk::Image& image : SwapChainImages)
    {
        imageViewCreateInfo.image = image;
        SwapChainImageViews.emplace_back(Device.GetLogicalDevice(), imageViewCreateInfo);
    }
}
