#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class CVEWindow;
class CVEDevice;

class CVESwapChain
{
public:    
    CVESwapChain(CVEDevice& device, const std::array<int, 2>& windowExtent);
    ~CVESwapChain();
    
    CVESwapChain(const CVESwapChain&) = delete;
    CVESwapChain& operator=(const CVESwapChain&) = delete;
    CVESwapChain(CVESwapChain&&) = delete;
    CVESwapChain& operator=(CVESwapChain&&) = delete;
    
    void RecreateSwapChain(const std::array<int, 2>& windowExtent);
    void CleanUp();
    
    const VkSurfaceFormatKHR& GetSurfaceFormat() const;
    const VkSwapchainKHR& GetSwapChain() const;
    const VkExtent2D& GetExtent() const;
    const std::vector<VkImage>& GetSwapChainImages() const;
    const std::vector<VkImageView>& GetSwapChainImageViews() const;
    
    VkResult AcquireNextImage(const uint32_t frameIndex, uint32_t* imageIndex);    
    void WaitForFences(const uint32_t frameIndex);
    void ResetFences(const uint32_t frameIndex);
    VkResult SubmitCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t frameIndex, const uint32_t imageIndex);    
    
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    
private:
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, std::array<int, 2> windowExtent);
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    uint32_t ChooseMinImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
    void CreateSwapChain(const std::array<int, 2>& windowExtent);
    void CreateImageViews();
    void CreateSyncObjects();
    
    CVEDevice& Device;
    VkSurfaceKHR& Surface;
    VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
    std::vector<VkImage> SwapChainImages;
    VkExtent2D Extent;
    VkSurfaceFormatKHR SurfaceFormat;
    std::vector<VkImageView> SwapChainImageViews;
    
    std::vector<VkSemaphore> PresentCompleteSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence> InFlightFences;
};
