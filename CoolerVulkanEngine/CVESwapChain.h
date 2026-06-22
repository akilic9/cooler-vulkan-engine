#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

class CVEWindow;
class CVEDevice;

class CVESwapChain
{
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    
    CVESwapChain(const CVEDevice& device, const CVEWindow& window);
    
    const vk::SurfaceFormatKHR& GetSurfaceFormat() const;
    const vk::raii::SwapchainKHR& GetSwapChain() const;
    const vk::Extent2D& GetExtent() const;
    const std::vector<vk::Image>& GetSwapChainImages() const;
    const std::vector<vk::raii::ImageView>& GetSwapChainImageViews() const;
    vk::ResultValue<uint32_t> AcquireNextImage(const vk::raii::Semaphore& presentCompleteSemaphore);
    
private:
    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const std::array<int, 2>& windowExtent);
    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    uint32_t ChooseMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);    
    void CreateImageViews();
    
    const CVEDevice& Device;
    const vk::raii::SurfaceKHR& Surface;
    vk::raii::SwapchainKHR SwapChain = nullptr;
    std::vector<vk::Image> SwapChainImages;
    vk::Extent2D Extent;
    vk::SurfaceFormatKHR SurfaceFormat;
    std::vector<vk::raii::ImageView> SwapChainImageViews;
};
