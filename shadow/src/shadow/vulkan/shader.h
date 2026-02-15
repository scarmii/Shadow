#pragma once

#include "shadow/events/eventDispatcher.h"
#include "shadow/vulkan/descriptorSetAllocator.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <functional>

namespace spirv_cross
{
	class Compiler;
	struct ShaderResources;
}

namespace Shadow
{
	class RenderPass;
	class Texture2D;
	class Mesh;
	class VertexBuffer;
	class UniformBuffer;
	class StorageBuffer;

	// identical to VkShaderStageFlagsBits
	enum class ShaderStage
	{
		None =     0,
		Vertex =   1 << 0,
		Fragment = 1 << 4,
		Compute =  1 << 5
	};
	SH_FLAG(ShaderStage)

		enum class ResourceType : uint8_t
	{
		UniformBuffer,
		SampledImage,
		SubpassInput,
		StorageBuffer,
		StorageImage
	};

	struct TextureResource
	{
		ResourceType type;
		ShaderStage shaderType;
		uint32_t set;
		uint32_t binding;
		uint32_t size;
		uint32_t arraySize;
	};

	struct Descriptor
	{
		VkDescriptorType type;
		uint32_t binding;
	};

	struct DescriptorSetLayout
	{
		VkDescriptorSetLayout layout;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	struct InputAttachment
	{
		std::string shaderName;
		std::string attachmentName;
	};

	struct ShaderResources
	{
		std::unordered_map<std::string, TextureResource> resources;
		Array<InputAttachment, 5> inputAttachments;
	    std::unordered_map<std::string, Ref<Texture2D>> renderpassInputs;
		std::array<DescriptorSetLayout, 4> descriptorSetLayouts;
	};

	class Shader
	{
	public:
		Shader(const std::string& name, const std::string vertPath, const std::string& fragPath);
		Shader(const std::string& name, const std::string computeSpv);
		Shader(const std::string& name, const std::string vertPath, const std::string& fragPath, const std::string computeSpv);
		~Shader();

		void setInput(const std::string& name, const Ref<UniformBuffer>& buffer);
		void setInput(const std::string& name, const Ref<VertexBuffer>& buffer);
		void setInput(const std::string& name, const Ref<StorageBuffer>& buffer);

		void setInput(const std::string& name, const std::string& inputAttachment);
		void setInput(const std::string& name, const Ref<Texture2D>& texture);
		void setInput(const std::string& name, uint32_t count, const Ref<Texture2D>* pTextures, uint32_t dstArrIndex = 0);
		void setInput(const std::string& name, const Mesh& mesh);

		bool isComputeShader() const { return m_stages & ShaderStage::Compute; }

		const std::string& getName() const { return m_name; }
		const TextureResource& getResource(const std::string& name) const { return m_resources.resources.find(name)->second; }
		ShaderStage getStages() const {return m_stages;}

		void updateDescriptorSets();
		inline void setRenderPass(RenderPass* pRenderpass) { m_pRenderPass = pRenderpass; }

		inline const VkShaderModule getVertexModule() const { return m_vertexShaderModule; }
		inline const VkShaderModule getFragModule() const { return m_fragmentShaderModule; }
		inline const VkShaderModule getComputeModule() const { return m_computeShaderModule; }
		inline const std::vector<uint32_t> getUsedDescriptorSets() const { return m_usedDescriptorSets; }
		inline const std::array<VkDescriptorSet, 4>& getDescriptorSets() const { return m_descriptorSets; }
		inline const std::array<VkDescriptorSetLayout, 4>& getDescriptorSetLayouts() const { return m_setLayouts; }
		inline const std::vector<VkPushConstantRange>& getPushConstantRanges() const { return m_pushConstantRanges; }

		static Ref<Shader> create(const std::string& name, const std::string vertPath, const std::string& fragPath) { return createRef<Shader>(name, vertPath, fragPath); }
		static Ref<Shader> create(const std::string& name, const std::string computeSpv) { return createRef<Shader>(name, computeSpv); }
		static Ref<Shader> create(const std::string& name, const std::string vertPath, const std::string& fragPath, const std::string computeSpv) { return createRef<Shader>(name, vertPath, fragPath, computeSpv); }
	private:
		std::vector<uint32_t> readFile(const std::string& filepath);
		VkShaderModule createShaderModule(const std::vector<uint32_t>& shaderCode);
		void retrieveShaderResources();
		void createDescriptorSetAllocator();
		void reflect(const spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& reflResources, VkShaderStageFlagBits shaderType);
	private:
		RenderPass* m_pRenderPass = nullptr;

		std::string m_name;
		ShaderStage m_stages = ShaderStage::None;

		VkShaderModule m_vertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_computeShaderModule = VK_NULL_HANDLE;

		std::vector<uint32_t> m_vertexShaderCode;
		std::vector<uint32_t> m_fragmentShaderCode;
		std::vector<uint32_t> m_computeShaderCode;

		Scope<DescriptorSetAllocator> m_descriptorSetAllocator;
		ShaderResources m_resources;

		std::array<VkDescriptorSet, 4> m_descriptorSets;
		std::array<VkDescriptorSetLayout, 4> m_setLayouts;
		std::vector<uint32_t> m_usedDescriptorSets;

		std::vector<VkPushConstantRange> m_pushConstantRanges;
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