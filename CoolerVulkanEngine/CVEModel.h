#pragma once
#include <memory>
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
    
    CVEModel(const CVEModel&) = delete;
    CVEModel& operator=(const CVEModel&) = delete;
    CVEModel(CVEModel&&) = delete;
    CVEModel& operator=(CVEModel&&) = delete;
    
    void LoadModel(const std::string& filePath);
    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    uint16_t AddLoadedTexture(const std::shared_ptr<CVETexture>& texture);
    
    const std::vector<std::shared_ptr<CVETexture>>& GetLoadedTextures() const;
    const std::string& GetFileDirectory() const;
    
private:
    void ProcessNode(aiNode* node, const aiScene* scene);
    
    CVEDevice& Device;
    
    std::string FileDirectory;
    
    std::vector<CVEMesh> Meshes;
    
    std::vector<std::shared_ptr<CVETexture>> LoadedTextures;
};
