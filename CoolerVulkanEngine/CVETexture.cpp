#include "CVETexture.h"

#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "CVEDevice.h"

CVETexture::CVETexture(CVEDevice& device, const std::string& filePath)
    : Device(device)
{
    CreateTexture(filePath);
}

CVETexture::~CVETexture()
{
    vkDestroyImage(Device.GetLogicalDevice(), TextureImage, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), TextureImageMemory, nullptr);
}

void CVETexture::CreateTexture(const std::string& filePath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }
    
    VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    Device.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propertyFlags, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(Device.GetLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(Device.GetLogicalDevice(), stagingBufferMemory);
    
    stbi_image_free(pixels);
    
    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VkCommandBuffer commandBuffer = Device.BeginSingleTimeCommands();
    TransitionImageLayout(commandBuffer, TextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    Device.CopyBufferToImage(commandBuffer, stagingBuffer, TextureImage, texWidth, texHeight);
    TransitionImageLayout(commandBuffer, TextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Device.EndSingleTimeCommands(commandBuffer);
    
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
    
    if (vkCreateImage(Device.GetLogicalDevice(), &imageInfo, nullptr, &TextureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image!");
    }
    
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(Device.GetLogicalDevice(), TextureImage, &memoryRequirements);
    
    uint32_t memoryTypeIndex = Device.FindMemoryType(memoryRequirements.memoryTypeBits, properties);
    
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize  = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
    
    if (vkAllocateMemory(Device.GetLogicalDevice(), &memoryAllocateInfo, nullptr, &TextureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory!");
    }
    
    if (vkBindImageMemory(Device.GetLogicalDevice(), TextureImage, TextureImageMemory, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to bind image memory!");
    }
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
