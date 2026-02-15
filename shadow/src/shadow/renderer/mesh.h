#pragma once

#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/pipeline.h"
		 
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
		Mesh(const std::string& path, uint32_t materialIndexOffset = 0);
		~Mesh();

		void setMaterialIndexOffset(uint32_t materialIndexOffset);
		void setPositionOffset(const glm::vec3& offset);
		void setTransform(const glm::mat4& transform);
		void updateVertexBuffer(const Ref<CommandBuffer>& cmdBuffer);
		void updateVertexBuffer(const CommandBuffer& cmdBuffer);

		inline const std::vector<Vertex>& getVertices() const { return m_vertices; }
		inline const Ref<VertexBuffer>& getVertexBuffer() const { return m_vertexBuffer; }
		inline const Ref<IndexBuffer>& getIndexBuffer() const { return m_indexBuffer; }
		inline const std::vector<Ref<Texture2D>>& getTextures() const { return m_textures; }
		inline const VertexInput& getVertexInput() const { return m_vertexInput; }
	private:
		void loadMaterials(const aiScene* pScene, std::vector<std::future<void>>& futures);
		void processNode(aiNode* node, const aiScene* scene, std::vector<uint32_t>& indices, uint32_t& offset);
		void processMesh(aiMesh* mesh, const aiScene* scene, std::vector<uint32_t>& indices, uint32_t& offset);
	private:
		std::string m_directory;
		std::vector<Ref<Texture2D>> m_textures;

		uint32_t m_indexCount;
		VertexInput m_vertexInput;
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		Ref<VertexBuffer> m_vertexBuffer;
		Ref<IndexBuffer> m_indexBuffer;
	};
}