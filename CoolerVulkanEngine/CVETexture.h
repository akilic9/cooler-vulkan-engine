#pragma once

#include <string>
#include <vulkan/vulkan_core.h>

class CVEDevice;

class CVETexture
{
public:
    CVETexture(CVEDevice& device, const std::string& filePath);
    ~CVETexture();
    
    VkDescriptorImageInfo GetDescriptorImageInfo();
private:
    void CreateTexture(const std::string& filePath);
    void CreateImage(uint32_t              width,
                     uint32_t              height,
                     VkFormat              format,
                     VkImageTiling         tiling,
                     VkImageUsageFlags     usage,
                     VkMemoryPropertyFlags properties);
    void CreateImageView();
    void CreateSampler();
    void TransitionImageLayout(VkCommandBuffer commandBuffer,
                               VkImage         image,
                               VkImageLayout   oldLayout,
                               VkImageLayout   newLayout);
    
    CVEDevice& Device;
    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
    VkImageView TextureImageView;
    VkSampler TextureSampler;
};
