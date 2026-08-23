#pragma once
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

struct CVEModelData;
struct CVEVertex;
class CVETexture;
class CVEMesh;
class CVEDevice;

class CVEModel
{
public:
    CVEModel(CVEDevice& device, const std::string& filePath);
    ~CVEModel();
    
    CVEModel(const CVEModel&) = delete;
    CVEModel& operator=(const CVEModel&) = delete;
    CVEModel(CVEModel&&) = delete;
    CVEModel& operator=(CVEModel&&) = delete;
    
    void LoadModel(const std::string& filePath);
    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    
private:
    void ProcessNode(aiNode* node, const aiScene* scene, CVEModelData& modelData);    
    static void ProcessVertices(std::vector<CVEVertex>& outVertices, const aiMesh* mesh);
    static void ProcessIndices(std::vector<uint32_t>& outIndices, const aiMesh* mesh);
    int32_t LoadTexture(const aiMesh* mesh, const aiScene* scene);
    bool CheckTextureLoaded(const std::string& fileName, int32_t& outTextureIndex);    
    
    void CreateVertexBuffer(const std::vector<CVEVertex>& vertices);
    void CreateIndexBuffer(const std::vector<uint32_t>& indices);
    
    CVEDevice& Device;    
    std::string FileDirectory;    
    
    VkBuffer VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory IndexBufferMemory = VK_NULL_HANDLE;
    
    std::vector<CVEMesh> Meshes;    
    std::vector<std::shared_ptr<CVETexture>> Textures;
};
