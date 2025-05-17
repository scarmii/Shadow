#pragma once

#include "Shadow/Renderer/Buffer.hpp"
#include "Shadow/Renderer/Pipeline.hpp"
		 
#include <glm/glm.hpp>
#include <assimp/scene.h>

#include <future>
#include <vector>

namespace Shadow
{
	class Mesh
	{
	public:
		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 texCoords;
			uint32_t materialIndex;
		};
	public:
		Mesh(const std::string& path);
		~Mesh();

		inline const Ref<VertexBuffer>& getVertexBuffer() const { return m_vertexBuffer; }
		inline const Ref<IndexBuffer>& getIndexBuffer() const { return m_indexBuffer; }

		inline const std::vector<Ref<Texture2D>>& getTextures() const { return m_textures; }
		inline const VertexInput& getVertexInput() const { return m_vertexInput; }
	private:
		void loadMaterials(const aiScene* pScene);
		void processNode(aiNode* node, const aiScene* scene);
		void processMesh(aiMesh* mesh, const aiScene* scene);
	private:
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		std::vector<Ref<Texture2D>> m_textures;

		Ref<VertexBuffer> m_vertexBuffer;
		Ref<IndexBuffer> m_indexBuffer;

		VertexInput m_vertexInput;

		std::vector<std::future<void>> m_futures;
		std::string m_directory;
	};
}