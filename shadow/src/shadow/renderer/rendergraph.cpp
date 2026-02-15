#include "shpch.h"
#include "shadow/renderer/rendergraph.h"
#include "shadow/renderer/renderer.h"

#include "shadow/vulkan/texture.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/shadowToVulkanTypes.h"

#include <iomanip>

namespace Shadow
{
	SH_FLAG_DEF(DrawPassFlags, uint8_t);

	RenderGraphPass::RenderGraphPass(const std::string& name, QueueType type)
		: m_name(name), m_type(type)
	{
	}

	RenderGraphPass::~RenderGraphPass()
	{
	}

	void RenderGraphPass::execute(const Ref<CommandBuffer>& cmdBuffer)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		for (const Barrier& barrier : m_barriers)
			vkCmdPipelineBarrier(cmdBuffer->getVkCommandBuffer(), barrier.srcStage, barrier.dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier.imageBarrier);

		begin(cmdBuffer);
		m_callback();
		end(cmdBuffer);
	}

	void RenderGraphPass::addBarrier(const Barrier& barrier)
	{
		m_barriers.emplace_back(barrier);
	}

	DrawPass::DrawPass(RenderGraph& renderGraph, const std::string& name, const Ref<GraphicsPipeline>& pipe, const Ref<Shader>& shader)
		: RenderGraphPass(name, QueueType::Graphics), m_renderGraph(renderGraph), m_graphicsPipeline(pipe), m_shader(shader), m_flags(DrawPassFlags::None)
	{
	}

	DrawPass::~DrawPass()
	{
	}

	void DrawPass::setCompatibleRenderPass(const Ref<RenderPass>& renderPass)
	{
		m_compatibleRenderPass = renderPass;
	}

	void DrawPass::begin(const Ref<CommandBuffer>& cmdBuffer)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		if (m_flags & DrawPassFlags::FirstSubpass)
			cmdBuffer->beginRenderPass(m_compatibleRenderPass);
		else
			cmdBuffer->nextSubpass();
	}

	void DrawPass::end(const Ref<CommandBuffer>& cmdBuffer)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		if (m_flags & DrawPassFlags::LastSubpass)
			cmdBuffer->endRenderPass();
	}

	ImageResource& DrawPass::addColorOutput(const std::string& name, const AttachmentInfo& info)
	{
		auto& res = m_renderGraph.getImageResource(name);
		res.setAttachmentInfo(info);
		res.addImageUsage(ImageUsage::ColorAttachment);
		res.setName(name);
		res.writtenInPass(this->getName());
		m_colorOutputs.emplace_back(&res);
		return res;
	}

	ImageResource& DrawPass::setDepthStencilOutput(const std::string& name, const AttachmentInfo& info)
	{
		auto& res = m_renderGraph.getImageResource(name);
		res.setAttachmentInfo(info);
		res.addImageUsage(ImageUsage::DepthAttachment);
		res.setName(name);
		res.writtenInPass(this->getName());

		m_flags |= DrawPassFlags::HasDepthStencil;
		m_depthStencilOutput = &res;
		return res;
	}

	void DrawPass::addInputAttachment(const std::string& name)
	{
		auto& res = m_renderGraph.getImageResource(name);
		res.addImageUsage(ImageUsage::SubpassInput);
		res.readInPass(this->getName());
		m_inputAttachments.emplace_back(name);
	}

	void DrawPass::addTextureInput(const std::string& name)
	{
		auto& res = m_renderGraph.getImageResource(name);
		res.addImageUsage(ImageUsage::SampledImage);
		res.readInPass(this->getName());
		m_textureInputs.emplace_back(&res);
	}

	ComputePass::ComputePass(RenderGraph& renderGraph, const std::string& name, const Ref<ComputePipeline>& pipe)
		: RenderGraphPass(name, QueueType::Compute), m_renderGraph(renderGraph), m_computePipeline(pipe)
	{
	}

	ComputePass::~ComputePass()
	{
	}

	void ComputePass::addStorageInput(const std::string& name)
	{
		auto& res = m_renderGraph.getImageResource(name);
		res.addImageUsage(ImageUsage::StorageImage);
		res.readInPass(this->getName());
		m_storageInputs.emplace_back(&res);
	}

	void ComputePass::addStorageOutput(const std::string& name)
	{
		auto& res = m_renderGraph.getImageResource(name);
		res.addImageUsage(ImageUsage::StorageImage);
		res.writtenInPass(this->getName());
		m_storageOutputs.emplace_back(&res);
	}

	void ComputePass::begin(const Ref<CommandBuffer>& cmdBuffer)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		cmdBuffer->beginComputePass(m_computePipeline);
	}

	void ComputePass::end(const Ref<CommandBuffer>& cmdBuffer)
	{
	}

	RenderGraph::~RenderGraph()
	{
	}

	Ref<DrawPass> RenderGraph::addDrawPass(const std::string& name, const Ref<GraphicsPipeline>& pipe, const Ref<Shader>& shader)
	{
		SH_ASSERT(pipe, "renderGraph: {handle = %x} => addDrawPass('%s'): pipeline hasn't been created yet", this, name.c_str());
		SH_ASSERT(shader, "renderGraph: {handle = %x} => addDrawPass('%s'): shader hasn't been created yet", this, name.c_str());
		pipe->setName(name);

		Ref<DrawPass> pass = createRef<DrawPass>(*this, name, pipe, shader);
		m_passIndices.emplace(name, static_cast<uint32_t>(m_passes.size()));
		m_passes.emplace_back(pass);
		return pass;
	}

	Ref<ComputePass> RenderGraph::addComputePass(const std::string& name, const Ref<ComputePipeline>& pipe)
	{
		SH_ASSERT(pipe, "renderGraph: {handle = %x} => addComputePass('%s'): pipeline hasn't been created yet", this, name.c_str());
		pipe->setName(name);

		Ref<ComputePass> pass = createRef<ComputePass>(*this, name, pipe);
		m_passIndices.emplace(name, static_cast<uint32_t>(m_passes.size()));
		m_passes.emplace_back(pass);
		return pass;
	}

	void RenderGraph::setup(const glm::vec4 clearColor)
	{
		FramebufferInfo framebufferInfo{};
		ShApp::get().getWindow().getFramebufferSize(framebufferInfo.width, framebufferInfo.height);
		setup(framebufferInfo, clearColor);
	}

	void RenderGraph::setup(const FramebufferInfo& info, const glm::vec4 clearColor)
	{
		std::vector<SubpassAttachment> attachments;
		attachments.reserve(5);

		setupPasses(attachments);
		createRenderPasses(info, clearColor);
		setTextures();
		transitionImageLayouts();
		setupBarriers();
	}

	void RenderGraph::execute(const Ref<CommandBuffer>& cmdBuffer)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		for (auto& pass : m_passes)
			pass->execute(cmdBuffer);
	}

	void RenderGraph::resizeFramebuffers(uint32_t newWidth, uint32_t newHeight)
	{
		for (Ref<RenderPass>& renderPass : m_renderPasses)
			renderPass->resizeFramebuffer(newWidth, newHeight);
	}

	ImageResource& RenderGraph::getImageResource(const std::string& name)
	{
		auto iter = m_imageResources.find(name);

		if (iter != m_imageResources.end())
			return iter->second;

		m_imageResources.emplace(name, ImageResource{});
		return m_imageResources[name];
	}

	void RenderGraph::setupBarriers()
	{
		auto it = m_passIndices.end();
		while (it != m_passIndices.begin())
		{
			if ((--it) == m_passIndices.begin())
				break;

			auto& pass = m_passes[it->second];
			auto& prevPass = m_passes[(--it)->second];
			it++;

			if (pass->getType() == QueueType::Graphics)
			{
				Ref<DrawPass> drawPass = as<DrawPass>(pass);

				for (auto textureInput : drawPass->getTextureInputs())
				{
					// there is no point to build a barrier for both graphics render passes, since they're synchronized via subpass dependencies
					if (textureInput->isWrittenInPass(prevPass->getName()) && prevPass->getType() == QueueType::Compute)
					{
						Barrier barrier{};
						barrier.srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
						barrier.dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

						initImageBarrier(textureInput, &barrier.imageBarrier);
						barrier.imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier.imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						barrier.imageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
						barrier.imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						pass->addBarrier(barrier);
					}
				}
			}
			else if (pass->getType() == QueueType::Compute)
			{
				Ref<ComputePass> computePass = as<ComputePass>(pass);

				for (auto storageInput : computePass->getStorageInputs())
				{
					if (storageInput->isWrittenInPass(prevPass->getName()) && prevPass->getType() == QueueType::Graphics)
					{
						Barrier barrier{};
						barrier.srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
						barrier.dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
						barrier.imageBarrier.oldLayout = as<DrawPass>(prevPass)->getCompatibleRenderPass()->getAttachmentDescription(storageInput->getName()).finalLayout;
						initImageBarrier(storageInput, &barrier.imageBarrier);

						if (storageInput->getImageUsage() & ImageUsage::ColorAttachment)
						{
							barrier.srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
							barrier.imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						}
						else if (storageInput->getImageUsage() & ImageUsage::DepthAttachment)
						{
							barrier.srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
							barrier.imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
						}

						barrier.dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
						barrier.imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						barrier.imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						pass->addBarrier(barrier);
					}

					// make sure that we write to the storage image after we've completed reading
					if (storageInput->isWrittenInPass(pass->getName()))
					{
						Barrier barrier{};
						barrier.srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
						barrier.dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

						initImageBarrier(storageInput, &barrier.imageBarrier);
						barrier.imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
						barrier.imageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
						barrier.imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier.imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						pass->addBarrier(barrier);
					}
				}
			}
		}
	}

	void RenderGraph::initImageBarrier(ImageResource* pRes, VkImageMemoryBarrier* pBarrier)
	{
		auto& image = pRes->getTexture();
		pBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		pBarrier->image = image->getVkImage();
		pBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		pBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		pBarrier->subresourceRange.aspectMask = image->getImageAspect();
		pBarrier->subresourceRange.layerCount = 1;
		pBarrier->subresourceRange.baseArrayLayer = 0;
		pBarrier->subresourceRange.levelCount = image->getMipLevelCount();
		pBarrier->subresourceRange.baseMipLevel = 0;
	}

	void RenderGraph::distributePasses()
	{
		std::vector<std::string> mergedPasses;
		mergedPasses.reserve(m_passIndices.size());

		for (auto it = m_passIndices.end(); it != m_passIndices.begin(); it--)
		{
			auto& pass = m_passes[(--it)->second];

			if (pass->getType() == QueueType::Graphics)
			{
				mergedPasses.emplace(mergedPasses.begin(), pass->getName());

				if (as<DrawPass>(pass)->getInputAttachments().empty())
				{
						mergePasses(mergedPasses);
						mergedPasses.clear();
				}
			}
			it++;
		}
	}

	void RenderGraph::mergePasses(const std::vector<std::string>& passNames)
	{
		MergedPass mergedPass{};

		Ref<DrawPass> firstPass = as<DrawPass>(m_passes[m_passIndices[passNames[0]]]);
		firstPass->setFirstSubpass(true);

		Ref<DrawPass> lastPass = as<DrawPass>(m_passes[m_passIndices[passNames[passNames.size() - 1]]]);
		lastPass->setLastSubpass(true);

		mergedPass.passNames = passNames;
		mergedPass.config.subpassCount = static_cast<uint32_t>(passNames.size());
		mergedPass.subpasses.resize(mergedPass.config.subpassCount);

		if (m_mergedPasses.empty());
			mergedPass.config.firstRenderpass = true;

		m_mergedPasses.emplace(m_mergedPasses.begin(), mergedPass);
	}

	void RenderGraph::setupPasses(std::vector<SubpassAttachment>& attachments)
	{
		distributePasses();

		auto& lastPass = m_passes.back();
		if (lastPass->getType() == QueueType::Graphics)
		{
			Ref<DrawPass> drawPass = as<DrawPass>(lastPass);

			for (auto& colorOut : drawPass->getColorOutputs())
				colorOut->addImageUsage(ImageUsage::SampledImage);
		}

		for (auto it = m_passIndices.begin(); it != m_passIndices.end(); it++)
		{
			Ref<RenderGraphPass>& pass = m_passes[it->second];
			if (pass->getType() == QueueType::Graphics)
			{
				Ref<RenderGraphPass>& nextPass = (it->second + 1 < m_passes.size() ? m_passes[it->second + 1] : nullptr);
				Ref<DrawPass> drawPass = as<DrawPass>(pass);
				auto& inputAttachments = drawPass->getInputAttachments();

				Subpass subpass{};
				subpass.shader = drawPass->getShader();
				subpass.pipeline = drawPass->getGraphicsPipeline();
				subpass.inputAttachmentCount = static_cast<uint32_t>(inputAttachments.size());
				subpass.pInputAttachments = inputAttachments.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(drawPass->getColorOutputs().size());
				subpass.pColorAttachments = attachments.data() + attachments.size();

				for (ImageResource* colorOutput : drawPass->getColorOutputs())
				{
					auto& attachmentInfo = colorOutput->getAttachmentInfo();
					SubpassAttachment colorAttachment{};
					colorAttachment.name = colorOutput->getName();
					colorAttachment.format = attachmentInfo.format;
					colorAttachment.loadOp = attachmentInfo.loadOp;
					colorAttachment.imageUsage = colorOutput->getImageUsage();

					if (nextPass)
					{
						if (colorOutput->isReadInPass(nextPass->getName()) && nextPass->getType() == QueueType::Graphics)
							colorAttachment.finalLayout = ImageLayout::ShaderReadOnlyOptimal;
					}

					if (colorOutput->getImageUsage() & ImageUsage::SampledImage)
						colorAttachment.finalLayout = ImageLayout::ShaderReadOnlyOptimal;

					if (m_passes.size() == 1 && attachmentInfo.loadOp == AttachmentLoadOp::Load)
						colorAttachment.initialLayout = colorAttachment.finalLayout;

					attachments.emplace_back(colorAttachment);
				}

				ImageResource* depthImageRes = drawPass->getDepthOutput();
				if (depthImageRes)
				{
					auto& attachmentInfo = depthImageRes->getAttachmentInfo();
					SubpassAttachment depthAttachment{};
					depthAttachment.name = depthImageRes->getName();
					depthAttachment.format = attachmentInfo.format;
					depthAttachment.loadOp = attachmentInfo.loadOp;
					depthAttachment.imageUsage = depthImageRes->getImageUsage();
					subpass.pDepthAttachment = attachments.data() + attachments.size();

					if (nextPass)
					{
						if (depthImageRes->isReadInPass(nextPass->getName()) && nextPass->getType() == QueueType::Graphics)
							depthAttachment.finalLayout = ImageLayout::ShaderReadOnlyOptimal;
					}
					attachments.emplace_back(depthAttachment);
				}

				for (auto& mergedPass : m_mergedPasses)
				{
					for (size_t i = 0; i < mergedPass.passNames.size(); i++)
					{
						if (pass->getName() == mergedPass.passNames[i])
						{
							mergedPass.subpasses[i] = subpass;
							break;
						}
					}
				}
			}
		}
	}

	void RenderGraph::createRenderPasses(const FramebufferInfo& info, const glm::vec4 clearColor)
	{
		for (size_t i = 0; i < m_mergedPasses.size(); i++)
		{
			MergedPass& mergedPass = m_mergedPasses[i];
			mergedPass.config.clearColor = clearColor;
			mergedPass.config.framebufferInfo.width = info.width;
			mergedPass.config.framebufferInfo.height = info.height;
			mergedPass.config.framebufferInfo.layers = info.layers;
			mergedPass.config.framebufferInfo.samples = info.samples;
			mergedPass.config.subpassCount = mergedPass.subpasses.size();
			mergedPass.config.pSubpasses = mergedPass.subpasses.data();
			m_renderPasses.emplace_back(RenderPass::create(mergedPass.config));

			for (const std::string& passName : mergedPass.passNames)
			{
				Ref<DrawPass> pass = as<DrawPass>(m_passes[m_passIndices[passName]]);
				pass->setCompatibleRenderPass(m_renderPasses.back());
			}
		}
	}

	void RenderGraph::setTextures()
	{
		for (auto& resource : m_imageResources)
		{
			ImageResource& image = resource.second;

			for (auto& renderPass : m_renderPasses)
			{
				const Ref<Texture2D>& texture = renderPass->getImage(image.getName());
				if (texture)
				{
					image.setTexture(texture);
					break;
				}
			}
		}
	}

	void RenderGraph::transitionImageLayouts()
	{
		for (auto& pass : m_passes)
		{
			if (pass->getType() == QueueType::Graphics)
			{
				Ref<DrawPass> drawPass = as<DrawPass>(pass);
				for (ImageResource* colorRes : drawPass->getColorOutputs())
				{
					auto& colorAttachment = drawPass->getCompatibleRenderPass()->getAttachmentDescription(colorRes->getName());

					if (m_passes.size() == 1 && colorAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
						transitionImageLayout(colorRes, colorAttachment.initialLayout);
				}

				if (drawPass->hasDepthAttachment())
				{
					ImageResource* depthRes = drawPass->getDepthOutput();
					auto& depthAttachment = drawPass->getCompatibleRenderPass()->getAttachmentDescription(depthRes->getName());

					if (m_passes.size() == 1 && depthAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
						transitionImageLayout(depthRes, depthAttachment.initialLayout);
				}
			}
		}
	}

	void RenderGraph::transitionImageLayout(ImageResource* imageRes, VkImageLayout initialLayout)
	{
		Device* device = VulkanContext::getDevice();
		VkCommandBuffer cmdBuffer = device->beginSingleTimeCmdBuffer(QueueType::Graphics);
		auto& image = imageRes->getTexture();

		switch (initialLayout)
		{
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				device->transitionImageLayout(cmdBuffer, image->getVkImage(), image->getFormat(),
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, image->getMipLevelCount());
				break;
			}
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				device->transitionImageLayout(cmdBuffer, image->getVkImage(), image->getFormat(),
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, image->getMipLevelCount());
				break;
			}
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				device->transitionImageLayout(cmdBuffer, image->getVkImage(), image->getFormat(),
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					0, VK_ACCESS_SHADER_READ_BIT, image->getMipLevelCount());
				break;
			}
			case VK_IMAGE_LAYOUT_GENERAL:
			{
				device->transitionImageLayout(cmdBuffer, image->getVkImage(), image->getFormat(),
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
					0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, image->getMipLevelCount());
				break;
			}
		}

		device->submitSingleTimeCmdBuffer(cmdBuffer, QueueType::Graphics);
	}
}