#include "shpch.h"

#include "shadow/core/core.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/renderPass.h"
#include "shadow/vulkan/shadowToVulkanTypes.h"
#include "shadow/vulkan/texture.h"
		 
#include <vector>

namespace Shadow
{
	SH_FLAG_DEF(PipelineStages, uint32_t);
	SH_FLAG_DEF(AccessFlags, uint32_t);

	GraphicsPipeline::GraphicsPipeline(const GraphicsPipeConfiguration& config)
		: m_config(config)
	{
		if (config.vertexInput)
			m_config.vertexInput = new VertexInput(*config.vertexInput);

		if (config.instanceInput)
			m_config.instanceInput = new VertexInput(*config.instanceInput);
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
		Device* device = VulkanContext::getDevice();

		if (!m_name.empty())
			SH_TRACE("destroying graphics pipeline: {name = %s; VkPipeline = %x; VkPipelineLayout = %x}", m_name.c_str(), m_pipeline, m_pipelineLayout);
		else
			SH_TRACE("destroying graphics pipeline: {VkPipeline = %x; VkPipelineLayout = %x}", m_name.c_str(), m_pipeline, m_pipelineLayout);

		if (m_config.vertexInput)
			delete m_config.vertexInput;

		if (m_config.instanceInput)
			delete m_config.instanceInput;

		vkDestroyPipelineLayout(device->getVkDevice(), m_pipelineLayout, nullptr);
		vkDestroyPipeline(device->getVkDevice(), m_pipeline, nullptr);
	}

	void GraphicsPipeline::setSubpass(uint32_t subpass, VkRenderPass renderpass)
	{
		m_subpass = subpass;
		m_vkRenderpass = renderpass;
	}

	void GraphicsPipeline::build(const RenderPass& renderpass)
	{
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		Shader* pShader = renderpass.getShader(m_subpass).get();
		build(pShader, (renderpass.hasDepthAttachment() ? &depthStencil : nullptr));
	}

	void GraphicsPipeline::build(Shader* shader, VkPipelineDepthStencilStateCreateInfo* depthStencilState)
	{
		if (m_pipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(VulkanContext::getDevice()->getVkDevice(), m_pipelineLayout, nullptr);

		if (m_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(VulkanContext::getDevice()->getVkDevice(), m_pipeline, nullptr);

		createPipelineLayout(shader);

		VkPipelineViewportStateCreateInfo viewportState{};
		initViewportState(&viewportState);

		std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;

		VkPipelineShaderStageCreateInfo shaderStages[2]{};

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = shader->getVertexModule();
		shaderStages[0].pName = "main";

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = shader->getFragModule();
		shaderStages[1].pName = "main";

		uint16_t location = 0;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> vertAttribDescriptions;

		processVertexDescription(m_config.vertexInput, 0, location,
			bindingDescriptions, vertAttribDescriptions, VK_VERTEX_INPUT_RATE_VERTEX);

		processVertexDescription(m_config.instanceInput, 1, location,
			bindingDescriptions, vertAttribDescriptions, VK_VERTEX_INPUT_RATE_INSTANCE);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertAttribDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = vertAttribDescriptions.data();

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		initColorBlendAttachmentState(m_config, &colorBlendAttachment);

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		VkPipelineMultisampleStateCreateInfo multisampling{};
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		initFixedFuncStagesInfos(m_config, &colorBlendAttachment, &inputAssembly, &rasterizer, &multisampling, &colorBlending);

		VkGraphicsPipelineCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.stageCount = 2;
		createInfo.pStages = shaderStages;
		createInfo.pVertexInputState = &vertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssembly;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizer;
		createInfo.pMultisampleState = &multisampling;
		createInfo.pDepthStencilState = depthStencilState;
		createInfo.pColorBlendState = &colorBlending;
		createInfo.pDynamicState = &dynamicState;
		createInfo.layout = m_pipelineLayout;
		createInfo.renderPass = m_vkRenderpass;
		createInfo.subpass = m_subpass;
		createInfo.basePipelineHandle = nullptr;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(VulkanContext::getDevice()->getVkDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline));
		SH_TRACE("created graphics pipeline: {VkPipeline = %x; VkPipelineLayout = %x}", m_pipeline, m_pipelineLayout);
	}

	void GraphicsPipeline::createPipelineLayout(Shader* shader)
	{
		if (shader)
		{
			auto& usedSets = shader->getUsedDescriptorSets();
			auto& setLayouts = shader->getDescriptorSetLayouts();

			m_pushConstantRanges = shader->getPushConstantRanges();
			m_descriptorSets.array = shader->getDescriptorSets();
			m_descriptorSets.size = usedSets.size();

			VkPipelineLayoutCreateInfo pipeLayoutInfo{};
			pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
			pipeLayoutInfo.pPushConstantRanges = m_pushConstantRanges.data();
			pipeLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
			pipeLayoutInfo.pSetLayouts = setLayouts.data();
			VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::getDevice()->getVkDevice(), &pipeLayoutInfo, nullptr, &m_pipelineLayout));
			return;
		}

		VkPipelineLayoutCreateInfo pipeLayoutInfo{};
		pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeLayoutInfo.pushConstantRangeCount = 0;
		pipeLayoutInfo.setLayoutCount = 0;
		VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::getDevice()->getVkDevice(), &pipeLayoutInfo, nullptr, &m_pipelineLayout));
	}

	void GraphicsPipeline::initViewportState(VkPipelineViewportStateCreateInfo* outViewportStateInfo) const
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)VulkanContext::getDevice()->getSwapchain()->getExtent().width;
		viewport.height = (float)VulkanContext::getDevice()->getSwapchain()->getExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = VulkanContext::getDevice()->getSwapchain()->getExtent();

		outViewportStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		outViewportStateInfo->viewportCount = 1;
		outViewportStateInfo->pViewports = &viewport;
		outViewportStateInfo->scissorCount = 1;
		outViewportStateInfo->pScissors = &scissor;
	}

	void GraphicsPipeline::initColorBlendAttachmentState(const GraphicsPipeConfiguration& stages, VkPipelineColorBlendAttachmentState* outAttachment) const
	{
		outAttachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		outAttachment->blendEnable = stages.states.blendState.blendEnable ? VK_TRUE : VK_FALSE;
		outAttachment->colorBlendOp = ShadowToVkCvt::blendOpToVk(stages.states.blendState.colorBlendOp);
		outAttachment->srcColorBlendFactor = ShadowToVkCvt::blendFactorToVk(stages.states.blendState.srcColorBlendFactor);
		outAttachment->dstColorBlendFactor = ShadowToVkCvt::blendFactorToVk(stages.states.blendState.dstColorBlendFactor);
		outAttachment->alphaBlendOp = ShadowToVkCvt::blendOpToVk(stages.states.blendState.alphaBlendOp);
		outAttachment->srcAlphaBlendFactor = ShadowToVkCvt::blendFactorToVk(stages.states.blendState.srcAlphaBlendFactor);
		outAttachment->dstAlphaBlendFactor = ShadowToVkCvt::blendFactorToVk(stages.states.blendState.dstAlphaBlendFactor);
	}

	void GraphicsPipeline::initFixedFuncStagesInfos(
		const GraphicsPipeConfiguration& stages,
		VkPipelineColorBlendAttachmentState* inAttachment,
		VkPipelineInputAssemblyStateCreateInfo* outInputAssembly,
		VkPipelineRasterizationStateCreateInfo* outRasterizer, 
		VkPipelineMultisampleStateCreateInfo* outMultisample,
		VkPipelineColorBlendStateCreateInfo* outColorBlending) const
	{
		outInputAssembly->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		outInputAssembly->topology = ShadowToVkCvt::primitiveTopologyToVk(stages.states.primitiveTopology);
		outInputAssembly->primitiveRestartEnable = stages.states.primitiveRestartEnable ? VK_TRUE : VK_FALSE;

		outRasterizer->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		outRasterizer->depthClampEnable = VK_FALSE;
		outRasterizer->rasterizerDiscardEnable = VK_FALSE;
		outRasterizer->polygonMode = ShadowToVkCvt::polygonModeToVk(stages.states.polygonMode);
		outRasterizer->lineWidth = stages.states.lineWidth;
		outRasterizer->cullMode = ShadowToVkCvt::cullModeToVk(stages.states.cullMode);
		outRasterizer->frontFace = ShadowToVkCvt::frontFaceToVk(stages.states.frontFace);
		outRasterizer->depthBiasEnable = VK_FALSE;

		outMultisample->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		outMultisample->sampleShadingEnable = VK_FALSE;
		outMultisample->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		outColorBlending->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		outColorBlending->logicOpEnable = VK_FALSE;
		outColorBlending->attachmentCount = 1;
		outColorBlending->pAttachments = inAttachment;
	}

	void GraphicsPipeline::processVertexDescription(const VertexInput* const description,
		uint16_t binding, uint16_t& offsetLocation,
		std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
		std::vector<VkVertexInputAttributeDescription>& vertAttribDescriptions,
		VkVertexInputRate inputRate)
	{
		if (description)
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = binding;
			bindingDescription.stride = description->getStride();
			bindingDescription.inputRate = inputRate;
			bindingDescriptions.emplace_back(bindingDescription);

			for (uint32_t i = 0; i < description->getVertexAttribs().size(); i++)
			{
				VkVertexInputAttributeDescription vertexAttribute{};
				vertexAttribute.binding = binding;
				vertexAttribute.location = offsetLocation++;
				vertexAttribute.format = ShadowToVkCvt::vertexAttributeTypeToVk(description->getVertexAttribs()[i].type);
				vertexAttribute.offset = description->getVertexAttribs()[i].offset;
				vertAttribDescriptions.emplace_back(vertexAttribute);
			}
		}
	}

	ComputePipeline::ComputePipeline(const Ref<Shader>& computeShader)
	{
		SH_ASSERT((computeShader->getStages() & ShaderStage::Compute), "failed to create a compute pipeline with a non-compute shader D:");

		Device* vulkanDevice = VulkanContext::getDevice();

		createPipelineLayout(computeShader);

		VkPipelineShaderStageCreateInfo shaderStageCI{};
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.module = computeShader->getComputeModule();
		shaderStageCI.pName = "main";

		VkComputePipelineCreateInfo pipeCI{};
		pipeCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeCI.layout = m_layout;
		pipeCI.stage = shaderStageCI;
		VK_CHECK_RESULT(vkCreateComputePipelines(vulkanDevice->getVkDevice(), VK_NULL_HANDLE, 1, &pipeCI, nullptr, &m_pipeline));
		SH_TRACE("created compute pipeline: {VkPipeline = %x; VkPipelineLayout = %x}", m_pipeline, m_layout);
	}

	ComputePipeline::~ComputePipeline()
	{
		Device* device = VulkanContext::getDevice();
		vkQueueWaitIdle(device->getGraphicsQueue());

		if (!m_name.empty())
			SH_TRACE("destroying compute pipeline: {name = %s; VkPipeline = %x; VkPipelineLayout = %x}", m_name.c_str(), m_pipeline, m_layout);
		else
			SH_TRACE("destroying compute pipeline: {VkPipeline = %x; VkPipelineLayout = %x}", m_pipeline, m_layout);

		vkDestroyPipelineLayout(device->getVkDevice(), m_layout, nullptr);
		vkDestroyPipeline(device->getVkDevice(), m_pipeline, nullptr);
	}

	void ComputePipeline::createPipelineLayout(const Ref<Shader>& shader)
	{
		if (shader)
		{
			auto& usedSets = shader->getUsedDescriptorSets();
			auto& setLayouts = shader->getDescriptorSetLayouts();

			m_descriptorSets.array = shader->getDescriptorSets();
			m_descriptorSets.size = usedSets.size();
			m_pushConstantRanges = shader->getPushConstantRanges();

			VkPipelineLayoutCreateInfo pipeLayoutCI{};
			pipeLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
			pipeLayoutCI.pPushConstantRanges = m_pushConstantRanges.data();
			pipeLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
			pipeLayoutCI.pSetLayouts = setLayouts.data();
			VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::getDevice()->getVkDevice(), &pipeLayoutCI, nullptr, &m_layout));
			return;
		}
	}
}
