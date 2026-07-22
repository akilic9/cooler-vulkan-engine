#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <string>
#include <vulkan/vulkan_core.h>

class CVEDevice;

class CVETexture
{
public:
    CVETexture(CVEDevice& device, const std::string& filePath);
    ~CVETexture();
    
private:
    void CreateTexture(const std::string& filePath);
    void CreateImage(uint32_t width, uint32_t height,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    
    void TransitionImageLayout(const VkCommandBuffer &commandBuffer,
                               const VkImage &image,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout);
    
    CVEDevice& Device;
    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
};
