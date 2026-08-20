#pragma once
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <vulkan/vulkan_core.h>
#include <glm.hpp>

struct CVEVertex;
class CVETexture;
class CVEDevice;

class CVEMesh
{
public:
    CVEMesh(CVEDevice& device, aiMesh* mesh, const aiScene* scene);
    ~CVEMesh();
    
private:
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void FillVertices(std::vector<CVEVertex>& outVertices, aiMesh* mesh);
    void FillIndices(std::vector<uint32_t>& outIndices, aiMesh* mesh);
    void CreateVertexBuffer(const std::vector<CVEVertex>& vertices);
    void CreateIndexBuffer(const std::vector<uint32_t>& indices);
    void BindBuffers();
    void Draw();
    
    CVEDevice& Device;
    
    VkBuffer VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory IndexBufferMemory = VK_NULL_HANDLE;
    
    std::vector<CVETexture> Textures;
    
    uint32_t IndexCount = 0;
    uint32_t VertexCount = 0;
};
