#pragma once

#include "Shadow/Core/Core.hpp"
#include "Shadow/Renderer/Buffer.hpp"

#include<unordered_map>

namespace Shadow
{
	class Mesh;
	class UniformBuffer;
	class StorageBuffer;
	class Texture2D;
	class Renderpass;

	enum class ShaderStage : uint8_t
	{
		None     = 0,
		Vertex   = 1 << 0,
		Fragment = 1 << 4,
		Compute  = 1 << 5
	};
	SH_FLAG(ShaderStage)

	enum class ResourceType : char
	{
		UniformBuffer,
		SampledImage,
		SubpassInput,
		StorageBuffer
	};

	struct Resource
	{
		ResourceType type;
		uint32_t set;
		uint32_t binding;
		uint32_t size;
		uint32_t arraySize;
	};

	struct ShaderResources
	{
		std::unordered_map<std::string, Resource> resources;

		virtual ~ShaderResources() = default;
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void writeDescriptorSet(const std::string& shaderName, const Ref<UniformBuffer>& buffer) = 0;
		virtual void writeDescriptorSet(const std::string& shaderName, const Ref<StorageBuffer>& buffer, bool acquireFromGraphicsQueue = false) = 0;

		virtual void writeDescriptorSet(const std::string& name, const Texture2D& texture) = 0;
		virtual void writeDescriptorSet(const std::string& name, const Ref<Texture2D>& texture) = 0;
		virtual void writeDescriptorSet(const std::string& name, uint32_t count, const Ref<Texture2D>* pTextures, uint32_t dstArrIndex = 0) = 0;
		virtual void writeDescriptorSet(const std::string& name, const Mesh& mesh) = 0;

		virtual bool isComputeShader() const = 0;

		virtual const std::string& getName() const = 0;
		virtual const Resource& getResource(const std::string& name) const = 0;
		virtual ShaderStage getStages() const = 0;

		static Ref<Shader> create(const std::string& name, const std::string& vertSpv, const std::string& fragSpv);
		static Ref<Shader> create(const std::string& name, const std::string& computeSpv);
		static Ref<Shader> create(const std::string& name, const std::string& vertSpv, const std::string& fragSpv, const std::string& computeSpv);
	protected:
		std::vector<uint32_t> readFile(const std::string& filepath);
	};

	class ShaderLibrary
	{
	public:
		void add(const Ref<Shader>& shader);
		Ref<Shader> load(const std::string& name, const std::string& vertSpv, const std::string& fragSpv);
		Ref<Shader> load(const std::string& name, const std::string& computeSpv);

		Ref<Shader> get(const std::string& name);
	private:
		inline bool exists(const std::string& name) const { return m_shaders.find(name) != m_shaders.end(); }
	private:
		std::unordered_map<std::string, Ref<Shader>> m_shaders;
	};
}
