#include "CVEModel.h"
#include <iostream>
#include <ostream>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

#include "CVEDevice.h"
#include "CVETexture.h"
#include "CVETypes.h"

CVEModel::CVEModel(CVEDevice& device, const std::string& filePath)
    : Device(device)
{
    FileDirectory = filePath.substr(0, filePath.find_last_of("/\\"));
    LoadModel(filePath);
}

CVEModel::~CVEModel()
{
    vkDestroyBuffer(Device.GetLogicalDevice(), VertexBuffer, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), VertexBufferMemory, nullptr);

    vkDestroyBuffer(Device.GetLogicalDevice(), IndexBuffer, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), IndexBufferMemory, nullptr);
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
    
    CVEModelData Data{};
    ProcessNode(Scene->mRootNode, Scene, Data);
    CreateVertexBuffer(Data.Vertices);
    CreateIndexBuffer(Data.Indices);
}

void CVEModel::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{

}

void CVEModel::ProcessNode(aiNode* node, const aiScene* scene, CVEModelData& modelData)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];        
        
        ProcessVertices(modelData.Vertices, mesh);
        uint32_t vertexCount = mesh->mNumVertices;
    
        uint32_t indexStart = (uint32_t)modelData.Indices.size();
        
        ProcessIndices(modelData.Indices, mesh);
        uint32_t indexCount = (uint32_t)modelData.Indices.size() - indexStart;
    
        int32_t textureIndex = LoadTexture(mesh, scene);
        
        Meshes.emplace_back(indexCount, vertexCount, indexStart, textureIndex);
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, modelData);
    }
}

void CVEModel::ProcessVertices(std::vector<CVEVertex>& outVertices, const aiMesh* mesh)
{
    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        CVEVertex vertex;
        
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                
        if (mesh->HasVertexColors(0))
        {
            const aiColor4D& colour = mesh->mColors[0][i];
            vertex.Colour = glm::vec4(colour.r, colour.g, colour.b, colour.a);
        }
        
        if (mesh->HasNormals())
        {
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        
        if (mesh->HasTextureCoords(0))
        {
            vertex.TexCoord0 = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        
        outVertices[i] = vertex;
    }
}

void CVEModel::ProcessIndices(std::vector<uint32_t>& outIndices, const aiMesh* mesh)
{
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++)
        {
            outIndices.push_back(face.mIndices[j]);
        }
    }
}

int32_t CVEModel::LoadTexture(const aiMesh* mesh, const aiScene* scene)
{
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        return -1;
    }
    
    aiString fileName;
    material->GetTexture(aiTextureType_DIFFUSE, 0, &fileName); // just diffuse for now
    
    int32_t textureIndex = -1;
    if (CheckTextureLoaded(fileName.C_Str(), textureIndex))
    {
        return textureIndex;
    }
        
    std::string path =  FileDirectory + fileName.C_Str();
    std::shared_ptr<CVETexture> newTexture;
    if (const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(fileName.C_Str()))
    {
        newTexture = CVETexture::LoadTexture(Device, path, embeddedTexture);
    }
    else
    {
        newTexture = CVETexture::LoadTexture(Device, path);
    }
        
    textureIndex = (uint16_t)Textures.size();
    Textures.push_back(newTexture);
    return textureIndex;
}

bool CVEModel::CheckTextureLoaded(const std::string& fileName, int32_t& outTextureIndex)
{
    for (uint32_t i = 0; i < Textures.size(); i++)
    {
        const std::shared_ptr<CVETexture>& loadedTexture = Textures[i];
        if (loadedTexture->GetFilePath().compare(fileName) == 0)
        {
            outTextureIndex = (int32_t)i;
            return true;
        }
    }
    
    return false;
}

void CVEModel::CreateVertexBuffer(const std::vector<CVEVertex>& vertices)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    Device.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propertyFlags, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(Device.GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(Device.GetLogicalDevice(), stagingBufferMemory);
    
    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    Device.CreateBuffer(bufferSize, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBuffer, VertexBufferMemory);

    Device.CopyBuffer(stagingBuffer, VertexBuffer, bufferSize);
    
    vkDestroyBuffer(Device.GetLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void CVEModel::CreateIndexBuffer(const std::vector<uint32_t>& indices)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    Device.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propertyFlags, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(Device.GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(Device.GetLogicalDevice(), stagingBufferMemory);

    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    Device.CreateBuffer(bufferSize, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, IndexBuffer, IndexBufferMemory);

    Device.CopyBuffer(stagingBuffer, IndexBuffer, bufferSize);
    
    vkDestroyBuffer(Device.GetLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), stagingBufferMemory, nullptr);
}
