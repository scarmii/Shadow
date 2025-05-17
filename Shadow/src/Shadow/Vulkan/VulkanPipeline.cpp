#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"

#include "Shadow/Vulkan/VulkanPipeline.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanShader.hpp"
#include "Shadow/Vulkan/VulkanUniformBuffer.hpp"
#include "Shadow/Vulkan/VulkanRenderpass.hpp"
#include "Shadow/Vulkan/ShadowToVulkanTypes.hpp"
#include "Shadow/Vulkan/VkTexture.hpp"
		 
#include <vector>

// TODO: dynamic rendering support 

namespace Shadow
{
	VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipeConfiguration& config, VkPipelineRenderingCreateInfo* pInfo)
		: m_config(config), m_renderpass(as<VulkanRenderpass>(config.renderpass))
	{
		SH_ASSERT(!(!config.useDynamicRendering && !config.renderpass), "");
		SH_ASSERT(config.shader, "graphics pipeline creation has been failed: shader that had been passed to create a graphics pipeline was nullptr :(");

		EventDispatcher::get().addReciever(SH_CALLBACK(VulkanGraphicsPipeline::onWindowResized));
		
		auto shader = as<VulkanShader>(config.shader);
		createPipelineLayout(shader);

 		VkPipelineShaderStageCreateInfo shaderStages[2]{}; 

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = shader->getVertexModule();
		shaderStages[0].pName = "main";

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = shader->getFragModule();
		shaderStages[1].pName = "main";


		VkPipelineViewportStateCreateInfo viewportState{};
		initViewportState(&viewportState);

		std::vector<VkDynamicState> dynamicStates =
		{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;

		uint16_t location = 0;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> vertAttribDescriptions;

		processVertexDescription(config.vertexInput, 0, location,
			bindingDescriptions, vertAttribDescriptions, VK_VERTEX_INPUT_RATE_VERTEX);

		processVertexDescription(config.instanceInput, 1, location,
			bindingDescriptions, vertAttribDescriptions, VK_VERTEX_INPUT_RATE_INSTANCE);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertAttribDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = vertAttribDescriptions.data();

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		initColorBlendAttachmentState(config, &colorBlendAttachment);

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		VkPipelineMultisampleStateCreateInfo multisampling{};
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		initFixedFuncStagesInfos(config, &colorBlendAttachment, &inputAssembly, &rasterizer, &multisampling, &colorBlending);

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;
		
		VulkanRenderpass* renderpass = (VulkanRenderpass*)config.renderpass.get();

		VkGraphicsPipelineCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.stageCount = 2;
		createInfo.pStages = shaderStages;
		createInfo.pVertexInputState = &vertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssembly;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizer;
		createInfo.pMultisampleState = &multisampling;

		if (renderpass)
			createInfo.pDepthStencilState = renderpass->hasDepthAttachment() ? &depthStencil : nullptr;
		else
			createInfo.pDepthStencilState = nullptr;

		createInfo.pColorBlendState = &colorBlending;
		createInfo.pDynamicState = &dynamicState;
		createInfo.layout = m_pipeLayout;
		createInfo.renderPass = renderpass ? renderpass->getVkRenderpass() : VK_NULL_HANDLE;
		createInfo.subpass = config.subpass;
		createInfo.basePipelineHandle = nullptr;
		createInfo.pNext = pInfo ? pInfo : nullptr;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(VulkanContext::getVulkanDevice()->getVkDevice(),
			VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline));
	}

	VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
	{
		VulkanDevice* pVkDevice = VulkanContext::getVulkanDevice();

		vkDestroyPipelineLayout(pVkDevice->getVkDevice(), m_pipeLayout, nullptr);
		vkDestroyPipeline(pVkDevice->getVkDevice(), m_pipeline, nullptr);
	}

	void VulkanGraphicsPipeline::setSubpassInput(const std::string& uniformName, uint32_t inputAttachment)
	{
		SH_PROFILE_FUNCTION();

		m_inputAttachments.setAt(InputAttachment{ uniformName, inputAttachment }, inputAttachment);
		auto& subpassInputRes = m_config.shader->getResource(uniformName);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_renderpass->getVulkanImage(inputAttachment).imageView;
		imageInfo.sampler = VK_NULL_HANDLE;

		VkWriteDescriptorSet descriptorWriter{};
		descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriter.dstSet = m_descriptorSets[subpassInputRes.set];
		descriptorWriter.dstBinding = subpassInputRes.binding;
		descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWriter.descriptorCount = 1;
		descriptorWriter.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(VulkanContext::getVulkanDevice()->getVkDevice(), 1, &descriptorWriter, 0, nullptr);
	}

	void VulkanGraphicsPipeline::setRenderpassInput(const std::string& shaderName, uint32_t imageIndex, const Ref<Renderpass>& src)
	{
		auto texture = as<VulkanTexture2D>(src->getOutput(imageIndex));
		auto& samplerRes = m_config.shader->getResource(shaderName);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture->getImage().imageView;
		imageInfo.sampler = texture->getSampler();

		VkWriteDescriptorSet writer{};
		writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writer.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writer.dstSet = m_descriptorSets[samplerRes.set];
		writer.dstBinding = samplerRes.binding;
		writer.dstArrayElement = 0;
		writer.descriptorCount = 1;
		writer.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(VulkanContext::getVulkanDevice()->getVkDevice(), 1, &writer, 0, nullptr);

		m_renderpassInputs[shaderName] = texture;
	}

	void VulkanGraphicsPipeline::createPipelineLayout(Ref<VulkanShader> shader)
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
			VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::getVulkanDevice()->getVkDevice(), &pipeLayoutInfo, nullptr, &m_pipeLayout));
			return;
		}

		VkPipelineLayoutCreateInfo pipeLayoutInfo{};
		pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeLayoutInfo.pushConstantRangeCount = 0;
		pipeLayoutInfo.setLayoutCount = 0;
		VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::getVulkanDevice()->getVkDevice(), &pipeLayoutInfo, nullptr, &m_pipeLayout));
	}

	void VulkanGraphicsPipeline::initViewportState(VkPipelineViewportStateCreateInfo* outViewportStateInfo) const
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)VulkanContext::getVulkanDevice()->getSwapchain()->getExtent().width;
		viewport.height = (float)VulkanContext::getVulkanDevice()->getSwapchain()->getExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = VulkanContext::getVulkanDevice()->getSwapchain()->getExtent();

		outViewportStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		outViewportStateInfo->viewportCount = 1;
		outViewportStateInfo->pViewports = &viewport;
		outViewportStateInfo->scissorCount = 1;
		outViewportStateInfo->pScissors = &scissor;
	}

	void VulkanGraphicsPipeline::initColorBlendAttachmentState(const GraphicsPipeConfiguration& stages, VkPipelineColorBlendAttachmentState* outAttachment) const
	{
		outAttachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		outAttachment->blendEnable = stages.states.blendState.blendEnable ? VK_TRUE : VK_FALSE;
		outAttachment->colorBlendOp = ShadowToVkCvt::shadowBlendOpToVk(stages.states.blendState.colorBlendOp);
		outAttachment->srcColorBlendFactor = ShadowToVkCvt::shadowBlendFactorToVk(stages.states.blendState.srcColorBlendFactor);
		outAttachment->dstColorBlendFactor = ShadowToVkCvt::shadowBlendFactorToVk(stages.states.blendState.dstColorBlendFactor);
		outAttachment->alphaBlendOp = ShadowToVkCvt::shadowBlendOpToVk(stages.states.blendState.alphaBlendOp);
		outAttachment->srcAlphaBlendFactor = ShadowToVkCvt::shadowBlendFactorToVk(stages.states.blendState.srcAlphaBlendFactor);
		outAttachment->dstAlphaBlendFactor = ShadowToVkCvt::shadowBlendFactorToVk(stages.states.blendState.dstAlphaBlendFactor);
	}

	void VulkanGraphicsPipeline::initFixedFuncStagesInfos(
		const GraphicsPipeConfiguration& stages,
		VkPipelineColorBlendAttachmentState* inAttachment,
		VkPipelineInputAssemblyStateCreateInfo* outInputAssembly,
		VkPipelineRasterizationStateCreateInfo* outRasterizer, 
		VkPipelineMultisampleStateCreateInfo* outMultisample,
		VkPipelineColorBlendStateCreateInfo* outColorBlending) const
	{
		outInputAssembly->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		outInputAssembly->topology = ShadowToVkCvt::shadowPrimitiveTopologyToVk(stages.states.primitiveTopology);
		outInputAssembly->primitiveRestartEnable = stages.states.primitiveRestartEnable ? VK_TRUE : VK_FALSE;

		outRasterizer->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		outRasterizer->depthClampEnable = VK_FALSE;
		outRasterizer->rasterizerDiscardEnable = VK_FALSE;
		outRasterizer->polygonMode = ShadowToVkCvt::shadowPolygonModeToVk(stages.states.polygonMode);
		outRasterizer->lineWidth = stages.states.lineWidth;
		outRasterizer->cullMode = ShadowToVkCvt::shadowCullModeToVk(stages.states.cullMode);
		outRasterizer->frontFace = ShadowToVkCvt::shadowFrontFaceToVk(stages.states.frontFace);
		outRasterizer->depthBiasEnable = VK_FALSE;

		outMultisample->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		outMultisample->sampleShadingEnable = VK_FALSE;
		outMultisample->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		outColorBlending->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		outColorBlending->logicOpEnable = VK_FALSE;
		outColorBlending->attachmentCount = 1;
		outColorBlending->pAttachments = inAttachment;
	}

	void VulkanGraphicsPipeline::processVertexDescription(const VertexInput* const description,
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
				vertexAttribute.format = ShadowToVkCvt::shadowVertexAttributeTypeToVk(description->getVertexAttribs()[i].type);
				vertexAttribute.offset = description->getVertexAttribs()[i].offset;
				vertAttribDescriptions.emplace_back(vertexAttribute);
			}
		}
	}

	bool VulkanGraphicsPipeline::onWindowResized(const WindowResizedEvent& e)
	{
		for (uint32_t i = 0; i < m_inputAttachments.size; i++)
		{
			auto& subpassInput = m_inputAttachments.array[i];
			setSubpassInput(subpassInput.shaderName, subpassInput.index);
			m_inputAttachments.size--;
		}

		for (auto& inImage : m_renderpassInputs)
			m_config.shader->writeDescriptorSet(inImage.first, inImage.second);

		return false;
	}

	VulkanComputePipeline::VulkanComputePipeline(const Ref<Shader>& computeShader)
	{
		SH_ASSERT((computeShader->getStages() & ShaderStage::Compute), "failed to create a compute pipeline with a non-compute shader D:");

		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();

		createPipelineLayout(computeShader);

		VkPipelineShaderStageCreateInfo shaderStageCI{};
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.module = as<VulkanShader>(computeShader)->getComputeModule();
		shaderStageCI.pName = "main";

		VkComputePipelineCreateInfo pipeCI{};
		pipeCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeCI.layout = m_layout;
		pipeCI.stage = shaderStageCI;
		VK_CHECK_RESULT(vkCreateComputePipelines(vulkanDevice->getVkDevice(), VK_NULL_HANDLE, 1, &pipeCI, nullptr, &m_pipeline));
	}

	VulkanComputePipeline::~VulkanComputePipeline()
	{
		VkDevice device = VulkanContext::getVulkanDevice()->getVkDevice();

		vkDestroyPipelineLayout(device, m_layout, nullptr);
		vkDestroyPipeline(device, m_pipeline, nullptr);
	}

	void VulkanComputePipeline::createPipelineLayout(const Ref<Shader>& shader)
	{
		if (shader)
		{
			auto vkShader = as<VulkanShader>(shader);
			auto& usedSets = vkShader->getUsedDescriptorSets();
			auto& setLayouts = vkShader->getDescriptorSetLayouts();

			m_descriptorSets.array = vkShader->getDescriptorSets();
			m_descriptorSets.size = usedSets.size();
			m_pushConstantRanges = vkShader->getPushConstantRanges();

			VkPipelineLayoutCreateInfo pipeLayoutCI{};
			pipeLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
			pipeLayoutCI.pPushConstantRanges = m_pushConstantRanges.data();
			pipeLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
			pipeLayoutCI.pSetLayouts = setLayouts.data();
			VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::getVulkanDevice()->getVkDevice(), &pipeLayoutCI, nullptr, &m_layout));
			return;
		}
	}
}
