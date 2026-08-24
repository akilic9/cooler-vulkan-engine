#pragma once
#include <array>
#include <glm.hpp>
#include <vulkan/vulkan_core.h>

struct CVEVertex
{
    glm::vec3 Position;
    glm::vec4 Colour{1.f};
    glm::vec3 Normal{1.f};
    glm::vec2 TexCoord0{0.f};
    
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

struct CVEGameObjectPushConstant
{
    glm::mat4 mModelMatrix{ 1.f };
    glm::mat4 mNormalMatrix{ 1.f };
};

struct CVETransform
{
    glm::vec3 Translation{ 0.f };
    glm::vec3 Scale{ 1.f };
    glm::vec3 Rotation{ 0.f }; // Degrees

    // Matrix corresponds to Translate * Ry * Rx * Rz * Scale
    // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
    // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix.
    glm::mat4 Mat4() const
    {
        const glm::vec3& radiansRot = glm::radians(Rotation);
        
        const float c3 = glm::cos(radiansRot.z);
        const float s3 = glm::sin(radiansRot.z);
        const float c2 = glm::cos(radiansRot.x);
        const float s2 = glm::sin(radiansRot.x);
        const float c1 = glm::cos(radiansRot.y);
        const float s1 = glm::sin(radiansRot.y);
        return glm::mat4
        {
            {
                Scale.x * (c1 * c3 + s1 * s2 * s3),
                Scale.x * (c2 * s3),
                Scale.x * (c1 * s2 * s3 - c3 * s1),
                0.0f,
            },
            {
                Scale.y * (c3 * s1 * s2 - c1 * s3),
                Scale.y * (c2 * c3),
                Scale.y * (c1 * c3 * s2 + s1 * s3),
                0.0f,
            },
            {
                Scale.z * (c2 * s1),
                Scale.z * (-s2),
                Scale.z * (c1 * c2),
                0.0f,
            },
            {Translation.x, Translation.y, Translation.z, 1.0f}
        };
    }

    glm::mat3 NormalMatrix() const
    {
        const glm::vec3& radiansRot = glm::radians(Rotation);
        
        const float c3 = glm::cos(radiansRot.z);
        const float s3 = glm::sin(radiansRot.z);
        const float c2 = glm::cos(radiansRot.x);
        const float s2 = glm::sin(radiansRot.x);
        const float c1 = glm::cos(radiansRot.y);
        const float s1 = glm::sin(radiansRot.y);

        const glm::vec3 inverseScale = 1.0f / Scale;

        return glm::mat3
        {
            {
                inverseScale.x * (c1 * c3 + s1 * s2 * s3),
                inverseScale.x * (c2 * s3),
                inverseScale.x * (c1 * s2 * s3 - c3 * s1),
            },
            {
                inverseScale.y * (c3 * s1 * s2 - c1 * s3),
                inverseScale.y * (c2 * c3),
                inverseScale.y * (c1 * c3 * s2 + s1 * s3),
            },
            {
                inverseScale.z * (c2 * s1),
                inverseScale.z * (-s2),
                inverseScale.z * (c1 * c2),
            }
        };
    }
};

struct CVEMesh
{
    uint32_t IndexCount;
    uint32_t VertexCount; 
    uint32_t FirstIndex;
    int32_t TextureIndex = -1;   
};

struct CVEModelData
{
    std::vector<CVEVertex> Vertices;
    std::vector<uint32_t> Indices;
};

struct CVEUniformBufferObject
{
    glm::mat4 View;
    glm::mat4 Projection;
};