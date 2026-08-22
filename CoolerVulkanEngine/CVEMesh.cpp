#include "CVEMesh.h"

#include "CVEDevice.h"
#include "CVEModel.h"
#include "CVETexture.h"
#include "CVETypes.h"

CVEMesh::CVEMesh(CVEDevice& device, CVEModel* owner, aiMesh* mesh, const aiScene* scene)
    : Device(device)
    , OwnerModel(owner)
{
    ProcessMesh(mesh, scene);
}

CVEMesh::~CVEMesh()
{
    vkDestroyBuffer(Device.GetLogicalDevice(), VertexBuffer, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), VertexBufferMemory, nullptr);

    vkDestroyBuffer(Device.GetLogicalDevice(), IndexBuffer, nullptr);
    vkFreeMemory(Device.GetLogicalDevice(), IndexBufferMemory, nullptr);
}

void CVEMesh::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<CVEVertex> vertices(mesh->mNumVertices);
    FillVertices(vertices, mesh);
    CreateVertexBuffer(vertices);
    
    std::vector<uint32_t> indices;
    FillIndices(indices, mesh);
    CreateIndexBuffer(indices);
    
    LoadTextures(mesh, scene);
}

void CVEMesh::FillVertices(std::vector<CVEVertex>& outVertices, const aiMesh* mesh)
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
        else
        {
            vertex.Colour = glm::vec4(1.0f);
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

void CVEMesh::FillIndices(std::vector<uint32_t>& outIndices, const aiMesh* mesh)
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

void CVEMesh::LoadTextures(const aiMesh* mesh, const aiScene* scene)
{
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    
    for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
    {
        aiString fileName;
        material->GetTexture(aiTextureType_DIFFUSE, i, &fileName);
        
        if (CheckTextureLoaded(fileName.C_Str()))
        {
            continue;
        }
        
        std::string path = OwnerModel->GetFileDirectory() + fileName.C_Str();
        std::shared_ptr<CVETexture> newTexture;
        if (const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(fileName.C_Str()))
        {
            newTexture = CVETexture::LoadTexture(Device, path, embeddedTexture);
        }
        else
        {
            newTexture = CVETexture::LoadTexture(Device, path);
        }
        
        uint16_t textureIndex = OwnerModel->AddLoadedTexture(newTexture);
        TextureIndices.push_back(textureIndex);
    }
}

bool CVEMesh::CheckTextureLoaded(const std::string& fileName)
{ 
    for (uint32_t i = 0; i < OwnerModel->GetLoadedTextures().size(); i++)
    {
        const std::shared_ptr<CVETexture>& loadedTexture = OwnerModel->GetLoadedTextures()[i];
        if (loadedTexture->GetFilePath().compare(fileName) == 0)
        {
            TextureIndices.push_back(i);
            return true;
        }
    }
    
    return false;
}

// TODO: function for vertex and index are similar
void CVEMesh::CreateVertexBuffer(const std::vector<CVEVertex>& vertices)
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

void CVEMesh::CreateIndexBuffer(const std::vector<uint32_t>& indices)
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

void CVEMesh::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
}
