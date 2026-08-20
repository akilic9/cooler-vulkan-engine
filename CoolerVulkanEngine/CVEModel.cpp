#include "CVEModel.h"
#include <iostream>
#include <ostream>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

#include "CVEMesh.h"

CVEModel::CVEModel(CVEDevice& device, const std::string& filePath)
    : Device(device)
    , FilePath(filePath)
{
}

CVEModel::~CVEModel()
{
}

void CVEModel::LoadModel()
{
    Assimp::Importer importer;
    const aiScene* Scene = importer.ReadFile(FilePath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (Scene == nullptr)
    {
        std::cerr << "Could not import model from: " << FilePath << std::endl;
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

void CVEModel::AddLoadedTexture(const CVETexture& texture)
{
    LoadedTextures.push_back(texture);
}

const std::vector<CVETexture>& CVEModel::GetLoadedTextures() const
{
    return LoadedTextures;
}

const std::string& CVEModel::GetFilePath() const
{
    return FilePath;
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
