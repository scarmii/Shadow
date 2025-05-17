#pragma once

#include "Shadow/Events/EventDispatcher.hpp"

#include "Shadow/Renderer/Shader.hpp"
#include "Shadow/Vulkan/DescriptorSetAllocator.hpp"
#include "Shadow/Vulkan/VulkanRenderpass.hpp"
#include "Shadow/Vulkan/VkTexture.hpp";

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

	struct VulkanShaderResources : public ShaderResources
	{
		std::array<DescriptorSetLayout, 4> descriptorSetLayouts;
	};

	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const std::string& name, const std::string vertPath, const std::string& fragPath);
		VulkanShader(const std::string& name, const std::string computeSpv);
		VulkanShader(const std::string& name, const std::string vertPath, const std::string& fragPath, const std::string computeSpv);
		virtual ~VulkanShader();

		virtual void writeDescriptorSet(const std::string& shaderName, const Ref<UniformBuffer>& buffer) override;
		virtual void writeDescriptorSet(const std::string& shaderName, const Ref<StorageBuffer>& buffer, bool acquireFromGraphicsQueue) override;

		virtual void writeDescriptorSet(const std::string& name, const Texture2D& texture) override;
		virtual void writeDescriptorSet(const std::string& name, const Ref<Texture2D>& texture) override;
		virtual void writeDescriptorSet(const std::string& name, uint32_t count, const Ref<Texture2D>* pTextures, uint32_t dstArrIndex = 0) override;
		virtual void writeDescriptorSet(const std::string& name, const Mesh& mesh) override;

		virtual bool isComputeShader() const override { return m_stages & ShaderStage::Compute; }

		virtual const std::string& getName() const override { return m_name; }
		virtual const Resource& getResource(const std::string& name) const  override { return m_resources->resources[name]; }
		virtual ShaderStage getStages() const override {return m_stages;}

		inline const VkShaderModule getVertexModule() const { return m_vertexShaderModule; }
		inline const VkShaderModule getFragModule() const { return m_fragmentShaderModule; }
		inline const VkShaderModule getComputeModule() const { return m_computeShaderModule; }
		inline const std::vector<uint32_t> getUsedDescriptorSets() const { return m_usedDescriptorSets; }
		inline const std::array<VkDescriptorSet, 4>& getDescriptorSets() const { return m_descriptorSets; }
		inline const std::array<VkDescriptorSetLayout, 4>& getDescriptorSetLayouts() const { return m_setLayouts; }
		inline const std::vector<VkPushConstantRange>& getPushConstantRanges() const { return m_pushConstantRanges; }
	private:
		VkShaderModule createShaderModule(const std::vector<uint32_t>& shaderCode);
		void retrieveShaderResources();
		void createDescriptorSetAllocator();
		void reflect(const spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& reflResources, VkShaderStageFlagBits shaderType);
	private:
		std::string m_name;
		ShaderStage m_stages = ShaderStage::None;

		VkShaderModule m_vertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_computeShaderModule = VK_NULL_HANDLE;

		std::vector<uint32_t> m_vertexShaderCode;
		std::vector<uint32_t> m_fragmentShaderCode;
		std::vector<uint32_t> m_computeShaderCode;

		Scope<DescriptorSetAllocator> m_descriptorSetAllocator;
		Scope<VulkanShaderResources> m_resources;

		std::array<VkDescriptorSet, 4> m_descriptorSets;
		std::array<VkDescriptorSetLayout, 4> m_setLayouts;
		std::vector<uint32_t> m_usedDescriptorSets;

		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};
}