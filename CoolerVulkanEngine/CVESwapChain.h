#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

class CVEWindow;
class CVEDevice;

class CVESwapChain
{
public:    
    CVESwapChain(CVEDevice& device, const CVEWindow& window);
    ~CVESwapChain();
    
    CVESwapChain(const CVESwapChain&) = delete;
    CVESwapChain& operator=(const CVESwapChain&) = delete;
    CVESwapChain(CVESwapChain&&) = delete;
    CVESwapChain& operator=(CVESwapChain&&) = delete;
    
    const vk::SurfaceFormatKHR& GetSurfaceFormat() const;
    const vk::raii::SwapchainKHR& GetSwapChain() const;
    const vk::Extent2D& GetExtent() const;
    const std::vector<vk::Image>& GetSwapChainImages() const;
    const std::vector<vk::raii::ImageView>& GetSwapChainImageViews() const;
    
    vk::ResultValue<uint32_t> AcquireNextImage(const uint32_t frameIndex);    
    void WaitForFences(const uint32_t frameIndex);
    void SubmitCommandBuffer(const vk::raii::CommandBuffer& commandBuffer, const uint32_t frameIndex, const uint32_t imageIndex);    
    
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    
private:
    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const std::array<int, 2>& windowExtent);
    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    uint32_t ChooseMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);    
    void CreateImageViews();
    void CreateSyncObjects();
    
    CVEDevice& Device;
    vk::raii::SurfaceKHR& Surface;
    vk::raii::SwapchainKHR SwapChain = nullptr;
    std::vector<vk::Image> SwapChainImages;
    vk::Extent2D Extent;
    vk::SurfaceFormatKHR SurfaceFormat;
    std::vector<vk::raii::ImageView> SwapChainImageViews;
    
    std::vector<vk::raii::Semaphore> PresentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> RenderFinishedSemaphores;
    std::vector<vk::raii::Fence> InFlightFences;
};
