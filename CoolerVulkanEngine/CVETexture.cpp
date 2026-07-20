#include "CVETexture.h"

#include "CVEDevice.h"

CVETexture::CVETexture(CVEDevice& device, const std::string& filePath)
    : Device(device)
{
    CreateTexture(filePath);
}

CVETexture::~CVETexture() {}

void CVETexture::CreateTexture(const std::string& filePath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }
    
    vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> stagingBufferAndMemory =
            Device.CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, propertyFlags);
    
    void* data = stagingBufferAndMemory.second.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferAndMemory.second.unmapMemory();
    
    stbi_image_free(pixels);
    
    vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    CreateImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, imageUsageFlags, vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    vk::raii::CommandBuffer commandBuffer = Device.BeginSingleTimeCommands();
    TransitionImageLayout(commandBuffer, TextureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    Device.CopyBufferToImage(commandBuffer, stagingBufferAndMemory.first, TextureImage, texWidth, texHeight);
    TransitionImageLayout(commandBuffer, TextureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    Device.EndSingleTimeCommands(commandBuffer);
}

void CVETexture::CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::ImageCreateInfo imageInfo {
        .imageType   = vk::ImageType::e2D,
        .format      = format,
        .extent      = vk::Extent3D{width, height, 1},
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = vk::SampleCountFlagBits::e1,
        .tiling      = tiling,
        .usage       = usage,
        .sharingMode = vk::SharingMode::eExclusive };
    
    TextureImage = vk::raii::Image(Device.GetLogicalDevice(), imageInfo);
    
    vk::MemoryRequirements memoryRequirements = TextureImage.getMemoryRequirements();
    
    uint32_t memoryTypeIndex = Device.FindMemoryType(memoryRequirements.memoryTypeBits, properties);
    
    vk::MemoryAllocateInfo memoryAllocateInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex };
    
    TextureImageMemory = vk::raii::DeviceMemory(Device.GetLogicalDevice(), memoryAllocateInfo);
    TextureImage.bindMemory(TextureImageMemory, 0);
}

void CVETexture::TransitionImageLayout(const vk::raii::CommandBuffer& commandBuffer,
                                       const vk::raii::Image& image,
                                       vk::ImageLayout oldLayout,
                                       vk::ImageLayout newLayout)
{
    vk::ImageMemoryBarrier barrier{
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image               = image,
        .subresourceRange {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1} };
    
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }
    
    commandBuffer.pipelineBarrier(sourceStage,
                                  destinationStage,
                                  {},
                                  {},
                                  {},
                                  barrier);
}
