#pragma once

#include "shadow/renderer/pipelineStatesConfig.h"
#include "shadow/renderer/vertexDescription.h"
#include "shadow/vulkan/shader.h"

#include<vulkan/vulkan.h>
#include<array>

namespace Shadow
{
	class RenderPass;
	class Shader;
	struct ShaderResources;

	// identical to VkPipelineStageFlagBits (bitfield)
	enum class PipelineStages
	{
		None = 0,
		TopOfPipe = 0x00000001,
		DrawIndirect = 0x00000002,
		VertexInput = 0x00000004,
		VertexShader = 0x00000008,
		FragmentShader = 0x00000080,
		EarlyFragmentTests = 0x00000100,
		LateFragmentTests = 0x00000200,
		ColorAttachmentOutput = 0x00000400,
		ComputeShader = 0x00000800,
		Transfer = 0x00001000,
		BottomOfPipe = 0x00002000
	};
	SH_FLAG(PipelineStages);

	// identical to VkAccessFlagBits (bitfield)
	enum class AccessFlags
	{
		None = 0,
		IndirectCommandRead = 0x00000001,
		IndexRead = 0x00000002,
		VertexAttributeRead = 0x00000004,
		UniformRead = 0x00000008,
		InputAttachmentRead = 0x00000010,
		ShaderRead = 0x00000020,
		ShaderWrite = 0x00000040,
		ColorAttachmentRead = 0x00000080,
		ColorAttachmentWrite = 0x00000100,
		DepthStencilAttachmentRead = 0x00000200,
		DepthStencilAttachmentWrite = 0x00000400,
		TransferRead = 0x00000800,
		TransferWrite = 0x00001000,
		HostRead = 0x00002000,
		HostWrite = 0x00004000,
		MemoryRead = 0x00008000,
		MemoryWrite = 0x00010000,
	};
	SH_FLAG(AccessFlags)

		struct GraphicsPipeConfiguration
	{
		const VertexInput* vertexInput = nullptr;
		const VertexInput* instanceInput = nullptr;
		GraphicsPipeStates states = GraphicsPipeStates();
	};

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(const GraphicsPipeConfiguration& config);
		~GraphicsPipeline();

		void setSubpass(uint32_t subpass, VkRenderPass renderpass);
		void build(const RenderPass& renderpass);
		void build(Shader* shader, VkPipelineDepthStencilStateCreateInfo* depthStencilState = nullptr);

		void setName(const std::string& name) { m_name = name; }
		inline const std::string& getName() const { return m_name; }

		const GraphicsPipeConfiguration& getConfiguration() const { return m_config; }
		inline const VkPipeline getVkPipeline() const { return m_pipeline; }
		inline const Array<VkDescriptorSet, 4>& getDescriptorSets() const { return m_descriptorSets; }
		inline const VkPipelineLayout getLayout() const { return m_pipelineLayout; }
		inline const std::vector<VkPushConstantRange>& getPushConstantRanges() const { return m_pushConstantRanges; }

		static Ref<GraphicsPipeline> create(const GraphicsPipeConfiguration& config) { return createRef<GraphicsPipeline>(config); }
	private:
		void createPipelineLayout(Shader* shader);

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
		std::string m_name;
		GraphicsPipeConfiguration m_config;
		uint32_t m_subpass;
		VkRenderPass m_vkRenderpass;

		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

		Array<VkDescriptorSet, 4> m_descriptorSets;
		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};

	class ComputePipeline
	{
	public:
		ComputePipeline(const Ref<Shader>& computeShader);
		~ComputePipeline();

		void setName(const std::string& name) { m_name = name; }
		inline const std::string& getName() const { return m_name; }

		inline VkPipeline getVkPipeline() const { return m_pipeline; }
		inline VkPipelineLayout getLayout() const { return m_layout; }
		inline const Array<VkDescriptorSet, 4>& getDescriptorSets() const { return m_descriptorSets; }
		inline const std::vector<VkPushConstantRange>& getPushConstantRanges() const { return m_pushConstantRanges; }

		static Ref<ComputePipeline> create(const Ref<Shader>& computeShader) { return createRef<ComputePipeline>(computeShader); }
	private:
		void createPipelineLayout(const Ref<Shader>& shader);
	private:
		std::string m_name;
		VkPipeline m_pipeline;
		VkPipelineLayout m_layout;
		Array<VkDescriptorSet, 4> m_descriptorSets;
		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};
}