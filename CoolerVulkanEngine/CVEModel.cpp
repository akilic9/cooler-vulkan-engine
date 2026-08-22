#include "CVEModel.h"
#include <iostream>
#include <ostream>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

#include "CVEMesh.h"

CVEModel::CVEModel(CVEDevice& device, const std::string& filePath)
    : Device(device)
{
    FileDirectory = filePath.substr(0, filePath.find_last_of("/\\"));
}

CVEModel::~CVEModel()
{
}

void CVEModel::LoadModel(const std::string& filePath)
{
    Assimp::Importer importer;
    const aiScene* Scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (!Scene)
    {
        std::cerr << "Could not import model from: " << filePath << std::endl;
        return;
    }

    ProcessNode(Scene->mRootNode, Scene);
}

void CVEModel::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
    for (CVEMesh& mesh : Meshes)
    {
        mesh.Draw(commandBuffer, pipelineLayout);
    }
}

uint16_t CVEModel::AddLoadedTexture(const std::shared_ptr<CVETexture>& texture)
{
    uint32_t index = (uint16_t)LoadedTextures.size();
    LoadedTextures.push_back(texture);
    return index;
}

const std::vector<std::shared_ptr<CVETexture>>& CVEModel::GetLoadedTextures() const
{
    return LoadedTextures;
}

const std::string& CVEModel::GetFileDirectory() const
{
    return FileDirectory;
}

void CVEModel::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Meshes.emplace_back(Device, this, mesh, scene);
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}
