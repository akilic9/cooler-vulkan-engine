#pragma once
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <vulkan/vulkan_core.h>
#include <glm.hpp>
#include <array>

class CVETexture;
class CVEDevice;

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

class CVEMesh
{
public:
    CVEMesh(CVEDevice& device, aiMesh* mesh, const aiScene* scene);
    ~CVEMesh();
    
private:
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void CreateVertexBuffers(std::vector<CVEVertex> vertices);
    void CreateIndexBuffers(std::vector<uint32_t> indices);
    void BindBuffers();
    void Draw();
    
    CVEDevice& Device;
    
    VkBuffer VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory IndexBufferMemory = VK_NULL_HANDLE;
    
    std::vector<CVEVertex> Vertices;
    std::vector<uint32_t> Indices;
    std::vector<CVETexture> Textures;
    
    uint32_t IndexCount = 0;
    uint32_t VertexCount = 0;
};
