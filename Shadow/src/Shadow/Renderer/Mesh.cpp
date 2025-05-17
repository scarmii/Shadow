#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"

#include "Shadow/Renderer/Mesh.hpp"
#include "Shadow/Renderer/Shader.hpp"
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Vulkan/VulkanBuffer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/BaseImporter.h>
#include <assimp/postprocess.h>

#include <chrono>

namespace Shadow
{
    Mesh::Mesh(const std::string& path)
        : m_vertexInput({ VertexAttribType::Vec3f, VertexAttribType::Vec3f, VertexAttribType::Vec2f, VertexAttribType::Uint })
    {
        m_directory = path.substr(0, path.find_last_of('/')+1);
        std::string meshExtension = path.substr(path.find_last_of('.') + 1, path.size() - 1);

        Assimp::Importer importer;
        const aiScene* pScene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast);

        SH_ASSERT((pScene && !(pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && pScene->mRootNode),
            "assimp::%s", importer.GetErrorString());

        processNode(pScene->mRootNode, pScene);
        loadMaterials(pScene);

        for (auto& future : m_futures)
            future.wait();
#ifndef OLD
        m_vertexBuffer = VertexBuffer::create(m_vertices.data(), sizeof(Vertex) * m_vertices.size(), sizeof(Vertex));
        m_indexBuffer = IndexBuffer::create(m_indices.data(), m_indices.size());
#else
        m_vertexBuffer = RenderBuffer::createVertexBuffer(m_vertices.data(), sizeof(Vertex) * m_vertices.size(), sizeof(Vertex));
        m_indexBuffer = RenderBuffer::createIndexBuffer(m_indices.data(), m_indices.size());
#endif
    }

    Mesh::~Mesh()
    {
    }

    static void loadMaterialAsync(std::vector<Ref<Texture2D>>& textures, const aiMaterial* pMaterial, const aiScene* pScene,
        uint32_t texIndex, const std::string& directory)
    {
        SH_PROFILE_FUNCTION();

        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString path;
            if (pMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), path) == AI_SUCCESS)
            {
                if (path.data[0] == '*')
                {
                    uint32_t index = static_cast<uint32_t>(path.data[1] - 48);
                    textures[texIndex] = Texture2D::create(reinterpret_cast<uint8_t*>(pScene->mTextures[index]->pcData), pScene->mTextures[index]->mWidth);
                    return;
                }

                std::string fullPath(directory + path.data);
                textures[texIndex] = Texture2D::create(fullPath);
                return;
            }
        }

        textures[texIndex] = Texture2D::create(1, 1);
        uint32_t whiteTexData = 0xffffffff;
        textures[texIndex]->setData(&whiteTexData);
    }

    void Mesh::loadMaterials(const aiScene* pScene)
    {
        m_textures.resize(pScene->mNumMaterials);

        for (uint32_t i = 0; i < pScene->mNumMaterials; i++)
        {
            m_textures[i] = nullptr;
            m_futures.emplace_back(std::move(std::async(std::launch::async, loadMaterialAsync, 
                std::ref(m_textures), pScene->mMaterials[i], pScene, i, std::cref(m_directory))));
        }
    }

    void Mesh::processNode(aiNode* node, const aiScene* scene)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
            processMesh(scene->mMeshes[node->mMeshes[i]], scene);        

        for (uint32_t i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    void Mesh::processMesh(aiMesh* mesh, const aiScene* scene)
    {
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            const aiVector3D& position = mesh->mVertices[i];
            const aiVector3D& normal = mesh->mNormals[i];

            Vertex vertex{};
            vertex.position = glm::vec3(position.x, position.y, position.z);
            vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
            vertex.texCoords = mesh->HasTextureCoords(0) ?
                glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
            vertex.materialIndex = mesh->mMaterialIndex;
            m_vertices.emplace_back(vertex);
        }

        static uint32_t offset = 0; // index buffer offset
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& face = mesh->mFaces[i];
            if (face.mNumIndices == 3)
                m_indices.insert(m_indices.end(), { face.mIndices[0] + offset, face.mIndices[1] + offset, face.mIndices[2] + offset });
        }
        offset = m_vertices.size();
    }
}