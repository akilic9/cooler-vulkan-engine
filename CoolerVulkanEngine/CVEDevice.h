#pragma once
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <vector>

class CVEWindow;

struct CVESwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> AvailableFormats;
    std::vector<VkPresentModeKHR> AvailablePresentModes;
};

class CVEDevice
{
public:
    CVEDevice(CVEWindow& inWindow);
    ~CVEDevice();
    
    CVEDevice(const CVEDevice&) = delete;
    CVEDevice& operator=(const CVEDevice&) = delete;
    CVEDevice(CVEDevice&&) = delete;
    CVEDevice& operator=(CVEDevice&&) = delete;
    
    const VkDevice& GetLogicalDevice() const;
    VkSurfaceKHR& GetSurface();
    CVESwapChainSupportDetails GetSwapChainSupportDetails() const;
    const VkCommandPool& GetCommandPool() const;
    VkQueue& GetGraphicsQueue();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    void CreateBuffer(VkDeviceSize          size,
                      VkBufferUsageFlags    usageFlags,
                      VkMemoryPropertyFlags propertyFlags,
                      VkBuffer&             outBuffer,
                      VkDeviceMemory&       outBufferMemory);
    
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    void CopyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size);
    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
private:
    void CreateVulkanInstance();
    void CreateDebugMessenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             type, 
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void*                                       pUserData);
    
    void PickPhysicalDevice();
    void FindQueueFamilyIndex();
    void CreateLogicalDevice();
    bool IsDeviceSuitable(const VkPhysicalDevice& physicalDevice);
    
    void CreateSurface();
    
    void CreateCommandPool();
    
    VkInstance VulkanInstance;
    VkDebugUtilsMessengerEXT DebugMessenger;
    
    CVEWindow& Window;
    VkPhysicalDevice PhysicalDevice;
    uint32_t QueueFamilyIndex = ~0;
    VkDevice LogicalDevice;
    VkQueue GraphicsQueue;
    
    VkSurfaceKHR Surface;
    
    VkCommandPool CommandPool;
};
