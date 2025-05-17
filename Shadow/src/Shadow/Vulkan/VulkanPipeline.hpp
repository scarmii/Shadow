#pragma once

#include "Shadow/Renderer/Pipeline.hpp"

#include<vulkan/vulkan.h>
#include<array>

namespace Shadow
{
	class VulkanRenderpass;
	class VulkanShader;
	struct VulkanShaderResources;

	class VulkanGraphicsPipeline : public GraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline(const GraphicsPipeConfiguration& config, VkPipelineRenderingCreateInfo* pInfo = nullptr);
		virtual ~VulkanGraphicsPipeline();

		virtual void setSubpassInput(const std::string& uniformName, uint32_t inputAttachment) override;
		virtual void setRenderpassInput(const std::string& shaderName, uint32_t imageIndex, const Ref<Renderpass>& src) override;

		virtual const GraphicsPipeConfiguration& getConfiguration() const { return m_config; }

		inline const Ref<VulkanRenderpass>& getVkRenderpass() const { return m_renderpass; }
		inline const VkPipeline getVkPipeline() const { return m_pipeline; }
		inline const Array<VkDescriptorSet, 4>& getDescriptorSets() const { return m_descriptorSets; }
		inline const VkPipelineLayout getLayout() const { return m_pipeLayout; }
		inline const std::vector<VkPushConstantRange>& getPushConstantRanges() const { return m_pushConstantRanges; }
	private:
		bool onWindowResized(const WindowResizedEvent& e);

		void createPipelineLayout(Ref<VulkanShader> shader);

		void initViewportState(VkPipelineViewportStateCreateInfo* outViewportState) const;
		void initColorBlendAttachmentState(const GraphicsPipeConfiguration& config, VkPipelineColorBlendAttachmentState* outAttachment) const;

		void initFixedFuncStagesInfos(
			const GraphicsPipeConfiguration& stages,
			VkPipelineColorBlendAttachmentState* inAttachment,
			VkPipelineInputAssemblyStateCreateInfo* outInputAssembly,
			VkPipelineRasterizationStateCreateInfo* outRasterizer,
			VkPipelineMultisampleStateCreateInfo* outMultisample,
			VkPipelineColorBlendStateCreateInfo* outColorBlending) const;

		void processVertexDescription(const VertexInput* const description,
			uint16_t binding, uint16_t& offsetLocation,
			std::vector<VkVertexInputBindingDescription>& bindingDescriptions, 
			std::vector<VkVertexInputAttributeDescription>& vertAttribDescriptions,
			VkVertexInputRate inputRate);
	private:
		GraphicsPipeConfiguration m_config;
		Ref<VulkanRenderpass> m_renderpass;

		VkPipeline m_pipeline;
		VkPipelineLayout m_pipeLayout;

		Array<VkDescriptorSet, 4> m_descriptorSets;
		std::vector<VkPushConstantRange> m_pushConstantRanges;

		Array<InputAttachment, 5> m_inputAttachments;
		std::unordered_map<std::string, Ref<Texture2D>> m_renderpassInputs;
	};

	class VulkanComputePipeline : public ComputePipeline
	{
	public:
		VulkanComputePipeline(const Ref<Shader>& computeShader);
		virtual ~VulkanComputePipeline();

		inline VkPipeline getVkPipeline() const { return m_pipeline; }
		inline VkPipelineLayout getLayout() const { return m_layout; }
		inline const Array<VkDescriptorSet, 4>& getDescriptorSets() const { return m_descriptorSets; }
		inline const std::vector<VkPushConstantRange>& getPushConstantRanges() const { return m_pushConstantRanges; }
	private:
		void createPipelineLayout(const Ref<Shader>& shader);
	private:
		VkPipeline m_pipeline;
		VkPipelineLayout m_layout;

		Array<VkDescriptorSet, 4> m_descriptorSets;
		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};
}