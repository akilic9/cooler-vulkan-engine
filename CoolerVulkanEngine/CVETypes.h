#pragma once
#include <array>
#include <glm.hpp>
#include <vulkan/vulkan_core.h>

struct CVEVertex
{
    glm::vec3 Position;
    glm::vec4 Colour;
    glm::vec3 Normal;
    glm::vec2 TexCoord0;
    
    static VkVertexInputBindingDescription GetBindingDesc()
    {
        VkVertexInputBindingDescription bindingDescriptions;
        bindingDescriptions.binding   = 0;
        bindingDescriptions.stride    = sizeof(CVEVertex);
        bindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }
    
    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDesc()
    {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0] = {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(CVEVertex, Position) };
        attributeDescriptions[1] = {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(CVEVertex, Colour) };
        attributeDescriptions[2] = {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(CVEVertex, Normal) };
        attributeDescriptions[3] = {.location = 3, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(CVEVertex, TexCoord0) };

        return attributeDescriptions;
    }
};
