#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <string>
#include <vector>

class CVEMesh;
class CVEDevice;

class CVEModel
{
public:
    CVEModel(CVEDevice& device, const std::string& filePath);
    ~CVEModel();
    
    void LoadModel();
    
private:
    void ProcessNode(aiNode* node, const aiScene* scene);
    
    CVEDevice& Device;
    
    const std::string FilePath;
    
    std::vector<CVEMesh> Meshes;
};
