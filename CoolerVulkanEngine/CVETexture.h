#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

struct aiTexture;
class CVEDevice;

class CVETexture
{
public:
    CVETexture(CVEDevice& device, const std::string& filePath, unsigned char* pixels, int width, int height);
    CVETexture(CVEDevice& device, const std::string& filePath, void* data, int width, int height, VkFormat format);
    ~CVETexture();
    
    CVETexture(const CVETexture&) = delete;
    CVETexture& operator=(const CVETexture&) = delete;
    CVETexture(CVETexture&&) = delete;
    CVETexture& operator=(CVETexture&&) = delete;
    
    static std::shared_ptr<CVETexture> LoadTexture(CVEDevice& device, const std::string& filePath);
    static std::shared_ptr<CVETexture> LoadTexture(CVEDevice& device,
                                                   const std::string& filePath,
                                                   const aiTexture* embeddedTexture);
    
    VkDescriptorImageInfo GetDescriptorImageInfo() const;
    
    const std::string& GetFilePath() const;
    
private:
    void CreateTexture(unsigned char* pixels, int width, int height, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    
    void WriteToStagingBuffer(const unsigned char* pixels,
                              int width,
                              int height,
                              VkBuffer& outBuffer,
                              VkDeviceMemory& outBufferMemory);    
    void CopyBuffer(const VkBuffer& stagingBuffer, int imgWidht, int imgHeight, VkFormat format);
    void DestroyStagingBuffer(const VkBuffer& stagingBuffer, const VkDeviceMemory& stagingBufferMemory) const;
    
    void CreateImage(uint32_t              width,
                     uint32_t              height,
                     VkFormat              format,
                     VkImageTiling         tiling,
                     VkImageUsageFlags     usage,
                     VkMemoryPropertyFlags properties);    
    void CreateImageView(VkFormat format);
    void CreateSampler();
    void CreateDescriptorImageInfo();
    
    void TransitionImageLayout(VkCommandBuffer commandBuffer,
                               VkImage         image,
                               VkImageLayout   oldLayout,
                               VkImageLayout   newLayout);
    
    CVEDevice& Device;
    VkImage TextureImage = VK_NULL_HANDLE;
    VkDeviceMemory TextureImageMemory = VK_NULL_HANDLE;
    VkImageView TextureImageView = VK_NULL_HANDLE;
    VkSampler TextureSampler = VK_NULL_HANDLE;
    VkDescriptorImageInfo DescriptorImageInfo;
    
    const std::string FilePath; // to avoid loading the same texture multiple times in models
};
