#pragma once

#include <string>
#include <vulkan/vulkan_core.h>

class CVEDevice;

class CVETexture
{
public:
    CVETexture(CVEDevice& device, const std::string& filePath);
    ~CVETexture();
    
    VkDescriptorImageInfo GetDescriptorImageInfo() const;
    
    const std::string& GetFilePath() const;
    
private:
    void CreateTexture();
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
    void CreateDescriptorImageInfo();
    
    CVEDevice& Device;
    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
    VkImageView TextureImageView;
    VkSampler TextureSampler;
    VkDescriptorImageInfo DescriptorImageInfo{};
    
    const std::string FilePath;
};
