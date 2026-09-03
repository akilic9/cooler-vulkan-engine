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
    
    TextureDescriptorSets.clear();
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
    
    CVEModelData data{};
    ProcessNode(Scene->mRootNode, Scene, data);
    CreateVertexBuffer(data.Vertices);
    CreateIndexBuffer(data.Indices);
}

void CVEModel::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VertexBuffer, offsets);
    
    vkCmdBindIndexBuffer(commandBuffer, IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    for (const CVEMesh& mesh : Meshes)
    {    
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, 
            &TextureDescriptorSets[mesh.TextureIndex], 0, nullptr);
    
        vkCmdDrawIndexed(commandBuffer, mesh.IndexCount, 1, mesh.FirstIndex, 0, 0);
    }
}

void CVEModel::Update()
{
    
}

uint32_t CVEModel::GetTextureCount() const
{
    return static_cast<uint32_t>(Textures.size());
}

void CVEModel::CreateTextureDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
{
    TextureDescriptorSets.resize(Textures.size());
    std::vector<VkDescriptorSetLayout> modelLayouts(Textures.size(), descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(modelLayouts.size());
    allocInfo.pSetLayouts        = modelLayouts.data();
    
    if (vkAllocateDescriptorSets(Device.GetLogicalDevice(), &allocInfo, TextureDescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set.");
    }

    for (int i = 0; i < Textures.size(); i++)
    {
        VkDescriptorImageInfo imageInfo = Textures[i]->GetDescriptorImageInfo();
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = TextureDescriptorSets[i];
        descriptorWrite.dstBinding      = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.pImageInfo      = &imageInfo;
        
        vkUpdateDescriptorSets(Device.GetLogicalDevice(), 1,
            &descriptorWrite, 0, nullptr);
    }
}

void CVEModel::ProcessNode(aiNode* node, const aiScene* scene, CVEModelData& modelData)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];        
        
        ProcessVertices(modelData.Vertices, mesh);
    
        uint32_t indexStart = (uint32_t)modelData.Indices.size();        
        ProcessIndices(modelData.Indices, mesh);
        uint32_t indexCount = (uint32_t)modelData.Indices.size() - indexStart;
    
        int32_t textureIndex = LoadTexture(mesh, scene);
        
        Meshes.emplace_back(indexCount, indexStart, textureIndex);
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
        
        outVertices.push_back(vertex);
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
    int32_t textureIndex = -1;
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    
    if (material->GetTextureCount(aiTextureType_DIFFUSE) <= 0)
    {
        return textureIndex;
    }
    
    aiString fileName;
    material->GetTexture(aiTextureType_DIFFUSE, 0, &fileName); // just diffuse for now
    
    if (CheckTextureLoaded(fileName.C_Str(), textureIndex))
    {
        return textureIndex;
    }
        
    std::string path =  FileDirectory + "/" + fileName.C_Str();
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
