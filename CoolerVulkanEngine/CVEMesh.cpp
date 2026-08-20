#include "CVEMesh.h"

#include "CVEDevice.h"
#include "CVETypes.h"

CVEMesh::CVEMesh(CVEDevice& device, aiMesh* mesh, const aiScene* scene)
    : Device(device)
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
}

void CVEMesh::FillVertices(std::vector<CVEVertex>& outVertices, aiMesh* mesh)
{
    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        CVEVertex vertex;
        
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                
        if (mesh->HasVertexColors(0))
        {
            const aiColor4D& colour = *mesh->mColors[0];
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

void CVEMesh::FillIndices(std::vector<uint32_t>& outIndices, aiMesh* mesh)
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

void CVEMesh::BindBuffers()
{
}

void CVEMesh::Draw()
{
}
