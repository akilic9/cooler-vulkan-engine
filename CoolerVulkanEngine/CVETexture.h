#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <string>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

class CVEDevice;

class CVETexture
{
public:
    CVETexture(CVEDevice& device, const std::string& filePath);
    ~CVETexture();
    
private:
    void CreateTexture(const std::string& filePath);
    void CreateImage(uint32_t width, uint32_t height,
                     vk::Format format, vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
    
    void TransitionImageLayout(const vk::raii::CommandBuffer &commandBuffer,
                               const vk::raii::Image &image,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout);
    
    CVEDevice& Device;
    vk::raii::Image TextureImage = nullptr;
    vk::raii::DeviceMemory TextureImageMemory = nullptr;
};
