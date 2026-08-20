#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

class CVETexture;
class CVEMesh;
class CVEDevice;

class CVEModel
{
public:
    CVEModel(CVEDevice& device, const std::string& filePath);
    ~CVEModel();
    
    void LoadModel();
    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    void AddLoadedTexture(const CVETexture& texture);
    
    const std::vector<CVETexture>& GetLoadedTextures() const;
    const std::string& GetFilePath() const;
    
private:
    void ProcessNode(aiNode* node, const aiScene* scene);
    
    CVEDevice& Device;
    
    const std::string FilePath;
    
    std::vector<CVEMesh> Meshes;
    
    std::vector<CVETexture> LoadedTextures;
};
