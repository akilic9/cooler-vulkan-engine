#include "CVETexture.h"

#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <stb_image.h>
#include <assimp/texture.h>

#include "CVEDevice.h"

CVETexture::CVETexture(CVEDevice& device, const std::string& filePath, unsigned char* pixels, int width, int height)
    : Device(device)
    , FilePath(filePath)
{
    if (!pixels)
    {
        std::cerr << "Could not load texture from: " << FilePath << std::endl;
        return;
    }
        
    CreateTexture(pixels, width, height);
    stbi_image_free(pixels);
}

CVETexture::CVETexture(CVEDevice& device, const std::string& filePath, void* data, int width, int height, VkFormat format)
    : Device(device)
    , FilePath(filePath)
{    
    CreateTexture(static_cast<unsigned char*>(data), width, height, VK_FORMAT_B8G8R8A8_SRGB);
}

std::shared_ptr<CVETexture> CVETexture::LoadTexture(CVEDevice& device, const std::string& filePath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    return std::make_shared<CVETexture>(device, filePath, pixels, texWidth, texHeight);
}

std::shared_ptr<CVETexture> CVETexture::LoadTexture(CVEDevice& device, const std::string& filePath, const aiTexture* embeddedTexture)
{
    void* data = embeddedTexture->pcData;
    
    if (embeddedTexture->mHeight == 0)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load_from_memory(static_cast<const stbi_uc*>(data), embeddedTexture->mWidth, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        return std::make_shared<CVETexture>(device, filePath, pixels, texWidth, texHeight);
    }
    
    return std::make_shared<CVETexture>(device, filePath, data, embeddedTexture->mWidth, embeddedTexture->mHeight, VK_FORMAT_B8G8R8A8_SRGB);
}

CVETexture::~CVETexture()
{
    vkDestroyImage(Device.GetLogicalDevice(), TextureImage, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), TextureImageMemory, nullptr);
    vkDestroyImageView(Device.GetLogicalDevice(), TextureImageView, nullptr);
    vkDestroySampler(Device.GetLogicalDevice(), TextureSampler, nullptr);
}

void CVETexture::CreateTexture(unsigned char* pixels, int width, int height, VkFormat format)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    WriteToStagingBuffer(pixels, width, height, stagingBuffer, stagingBufferMemory);
    CopyBuffer(stagingBuffer, width, height, format);
    DestroyStagingBuffer(stagingBuffer, stagingBufferMemory);
    
    CreateImageView(format);
    CreateSampler();
    CreateDescriptorImageInfo();
}

void CVETexture::WriteToStagingBuffer(const unsigned char* pixels, int width, int height, VkBuffer& outBuffer, VkDeviceMemory& outBufferMemory)
{
    VkDeviceSize imageSize = width * height * 4;
    VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    Device.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propertyFlags, outBuffer, outBufferMemory);
    
    void* data;
    vkMapMemory(Device.GetLogicalDevice(), outBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(Device.GetLogicalDevice(), outBufferMemory);
}

void CVETexture::CopyBuffer(const VkBuffer& stagingBuffer, int imgWidht, int imgHeight, VkFormat format)
{
    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    CreateImage(imgWidht, imgHeight, format, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VkCommandBuffer commandBuffer = Device.BeginSingleTimeCommands();
    TransitionImageLayout(commandBuffer, TextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    Device.CopyBufferToImage(commandBuffer, stagingBuffer, TextureImage, imgWidht, imgHeight);
    TransitionImageLayout(commandBuffer, TextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Device.EndSingleTimeCommands(commandBuffer);
}

void CVETexture::DestroyStagingBuffer(const VkBuffer& stagingBuffer, const VkDeviceMemory& stagingBufferMemory) const
{
    vkDestroyBuffer(Device.GetLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void CVETexture::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType   = VK_IMAGE_TYPE_2D;
    imageInfo.format      = format;
    imageInfo.extent      = {width, height, 1};
    imageInfo.mipLevels   = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling      = tiling;
    imageInfo.usage       = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    Device.CreateImageFromInfo(imageInfo, properties, TextureImage, TextureImageMemory);
}

void CVETexture::CreateImageView(VkFormat format)
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = TextureImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format   = format;
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;
    
    Device.CreateImageViewFromInfo(createInfo, TextureImageView);
}

void CVETexture::CreateSampler()
{    
    VkPhysicalDeviceProperties physicalDeviceProperties {};
    Device.GetPhysicalDeviceProperties(physicalDeviceProperties);
    
    VkSamplerCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter               = VK_FILTER_LINEAR;
    createInfo.minFilter               = VK_FILTER_LINEAR;
    createInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable        = VK_TRUE;
    createInfo.maxAnisotropy           = physicalDeviceProperties.limits.maxSamplerAnisotropy;
    createInfo.compareEnable           = VK_FALSE;
    createInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    createInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.mipLodBias              = 0.f;
    createInfo.minLod                  = 0.f;
    createInfo.maxLod                  = 0.f;
    
    if (vkCreateSampler(Device.GetLogicalDevice(), &createInfo, nullptr, &TextureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create sampler!");
    }
}

void CVETexture::CreateDescriptorImageInfo()
{
    DescriptorImageInfo.sampler     = TextureSampler;
    DescriptorImageInfo.imageView   = TextureImageView;
    DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void CVETexture::TransitionImageLayout(VkCommandBuffer commandBuffer,
                                       VkImage         image,
                                       VkImageLayout   oldLayout,
                                       VkImageLayout   newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                   = oldLayout;
    barrier.newLayout                   = newLayout;
    barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                       = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage,
                         destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
}

VkDescriptorImageInfo CVETexture::GetDescriptorImageInfo() const
{
    return DescriptorImageInfo;
}

const std::string& CVETexture::GetFilePath() const
{
    return FilePath;
}
