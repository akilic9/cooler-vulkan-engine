#include "CVEMesh.h"

CVEMesh::CVEMesh(CVEDevice& device, aiMesh* mesh, const aiScene* scene)
    : Device(device)
{
    ProcessMesh(mesh, scene);
}

CVEMesh::~CVEMesh()
{
}

void CVEMesh::ProcessMesh(aiMesh* mesh, const aiScene* scene)
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
        
        Vertices.push_back(vertex);
    }
    
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++)
        {
            Indices.push_back(face.mIndices[j]);
        }
    }
}
