#pragma once
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <vulkan/vulkan_core.h>
#include <glm.hpp>

class CVEModel;
struct CVEVertex;
class CVETexture;
class CVEDevice;

class CVEMesh
{
public:
    CVEMesh(CVEDevice& device, CVEModel* owner, aiMesh* mesh, const aiScene* scene);
    ~CVEMesh();
    
    CVEMesh(const CVEMesh&) = delete;
    CVEMesh& operator=(const CVEMesh&) = delete;
    CVEMesh(CVEMesh&&) = delete;
    CVEMesh& operator=(CVEMesh&&) = delete;
    
    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    
private:
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void FillVertices(std::vector<CVEVertex>& outVertices, const aiMesh* mesh);
    void FillIndices(std::vector<uint32_t>& outIndices, const aiMesh* mesh);
    void LoadTextures(const aiMesh* mesh, const aiScene* scene);
    bool CheckTextureLoaded(const std::string& fileName);
    void CreateVertexBuffer(const std::vector<CVEVertex>& vertices);
    void CreateIndexBuffer(const std::vector<uint32_t>& indices);
    
    CVEDevice& Device;
    CVEModel* OwnerModel;
    
    VkBuffer VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory IndexBufferMemory = VK_NULL_HANDLE;
    
    std::vector<uint32_t> TextureIndices;
    
    uint32_t IndexCount = 0;
    uint32_t VertexCount = 0;
};
