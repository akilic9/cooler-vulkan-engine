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

void CVEModel::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Meshes.emplace_back(Device, mesh, scene);
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}
