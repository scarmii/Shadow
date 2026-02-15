#include "shpch.h"
#include "shadow/core/core.h"
#include "shadow/core/shApp.h"
#include "shadow/renderer/renderer.h"

#include "shadow/vulkan/renderPass.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/shader.h"
#include "shadow/vulkan/shadowToVulkanTypes.h"
#include "shadow/vulkan/texture.h"
		 
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

namespace Shadow
{
	RenderPass::RenderPass(const RenderPassConfig& config, const std::string& name)
		: m_config(config), m_name(name)
	{
		EventDispatcher::get().addReciever(SH_ON_EVENT_FN(onWidnowResized, WindowResizedEvent));

		for (uint32_t i = 0; i < config.subpassCount; i++)
			addSubpass(config.pSubpasses[i], i);

		createRenderpass();
		createFramebuffer();

		setClearColor(config.clearColor);

		for (size_t i = 0; i < m_subpassDescriptions.size(); i++)
		{
			Ref<GraphicsPipeline>& pipeline = m_subpasses[i].pipeline;
			pipeline->setSubpass(i, m_renderPass);
			pipeline->build(*this);

			m_subpasses[i].shader->setRenderPass(this);
		}
	}

	RenderPass::~RenderPass()
	{
		VulkanContext::getDevice()->waitIdle();
		vkDestroyFramebuffer(VulkanContext::getDevice()->getVkDevice(), m_framebuffer, nullptr);
		vkDestroyRenderPass(VulkanContext::getDevice()->getVkDevice(), m_renderPass, nullptr);
	}

	void RenderPass::addSubpass(const Subpass& subpass, uint32_t index)
	{
		m_subpasses[index].pipeline = subpass.pipeline;
		m_subpasses[index].shader = subpass.shader;

		VkAttachmentReference* pColorRefs = nullptr;

		if (subpass.colorAttachmentCount > 0)
			pColorRefs = &m_attachmentRefs[m_clearValues.size()];

		VkAttachmentReference* pInputRefs = subpass.inputAttachmentCount > 0 ?
			&m_inputAttachmentRefs[m_attachmentIndices[*subpass.pInputAttachments]] : nullptr;

		VkAttachmentReference* pDepthRef = subpass.pDepthAttachment ?
			&m_attachmentRefs[m_clearValues.size() + subpass.colorAttachmentCount] : nullptr;

		ImageUsage subpassFlags = ImageUsage::None;

		for (uint32_t i = 0; i < subpass.colorAttachmentCount; i++)
		{
			size_t currAttachment = m_clearValues.size();
			m_attachmentIndices[subpass.pColorAttachments[i].name] = currAttachment;

			VkAttachmentDescription& colorAttachment = m_attachments[currAttachment];
			colorAttachment.format = ShadowToVkCvt::imageFormatToVk(subpass.pColorAttachments[i].format);
			colorAttachment.flags = VK_DEPENDENCY_BY_REGION_BIT;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = ShadowToVkCvt::attachmentLoadOpToVk(subpass.pColorAttachments[i].loadOp);
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = static_cast<VkImageLayout>(subpass.pColorAttachments[i].initialLayout);

			colorAttachment.finalLayout = subpass.pColorAttachments[i].finalLayout == ImageLayout::Undefined ?
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : static_cast<VkImageLayout>(subpass.pColorAttachments[i].finalLayout);

			pColorRefs[i] = { static_cast<uint32_t>(currAttachment), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			m_clearBits |= 1 << currAttachment;
			m_clearValues.emplace_back(VkClearValue{ 1.0f,0.0f });
			 
			m_images[pColorRefs[i].attachment] = Texture2D::create(m_config.framebufferInfo.width, m_config.framebufferInfo.height,
				subpass.pColorAttachments[i].imageUsage, subpass.pColorAttachments[i].format, subpass.pColorAttachments[i].sampler, subpass.pColorAttachments[i].initialLayout);

			if (subpass.pColorAttachments[i].imageUsage & ImageUsage::SubpassInput)
				m_inputAttachmentRefs[pColorRefs[i].attachment] = { pColorRefs[i].attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}

		if (subpass.pDepthAttachment)
		{
			size_t currAttachment = m_clearValues.size();
			m_attachmentIndices[subpass.pDepthAttachment->name] = currAttachment;
			m_hasDepthAttachment = true;

			VkFormat requestedFormat = ShadowToVkCvt::imageFormatToVk(subpass.pDepthAttachment->format);
			VkFormat retrievedFormat = VulkanContext::getDevice()->findSupportedFormat(
				{ requestedFormat, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);

			VkAttachmentDescription& depthAttachment = m_attachments[currAttachment];
			depthAttachment.flags = VK_DEPENDENCY_BY_REGION_BIT;
			depthAttachment.format = retrievedFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = ShadowToVkCvt::attachmentLoadOpToVk(subpass.pDepthAttachment->loadOp);
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = static_cast<VkImageLayout>(subpass.pDepthAttachment->initialLayout);

			depthAttachment.finalLayout = subpass.pDepthAttachment->finalLayout == ImageLayout::Undefined ?
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : static_cast<VkImageLayout>(subpass.pDepthAttachment->finalLayout);

			*pDepthRef = { static_cast<uint32_t>(currAttachment), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

			m_clearValues.emplace_back(VkClearValue{ 1.0f,0.0f });
			m_images[pDepthRef->attachment] = createRef<Texture2D>(m_config.framebufferInfo.width, m_config.framebufferInfo.height,
				subpass.pDepthAttachment->imageUsage, subpass.pDepthAttachment->format, subpass.pDepthAttachment->sampler, subpass.pDepthAttachment->initialLayout);

			if (subpass.pDepthAttachment->imageUsage & ImageUsage::SubpassInput)
				m_inputAttachmentRefs[pDepthRef->attachment] = { pDepthRef->attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}

		VkSubpassDescription subpassDescription{};
		subpassDescription.flags = 0;
		subpassDescription.colorAttachmentCount = subpass.colorAttachmentCount;
		subpassDescription.pColorAttachments = pColorRefs;
		subpassDescription.pDepthStencilAttachment = subpass.pDepthAttachment ? pDepthRef : nullptr;
		subpassDescription.inputAttachmentCount = subpass.inputAttachmentCount;
		subpassDescription.pInputAttachments = subpass.inputAttachmentCount ? pInputRefs : nullptr;
		m_subpassDescriptions.emplace_back(subpassDescription);

		setupSubpassDependency(subpass);
	}

	void RenderPass::setupSubpassDependency(const Subpass& subpass)
	{
		uint32_t dstSubpass = static_cast<uint32_t>(m_subpassDescriptions.size()) - 1;

		VkSubpassDependency dep{};
		dep.srcSubpass = dstSubpass < 1 ? VK_SUBPASS_EXTERNAL : dstSubpass - 1;
		dep.dstSubpass = dstSubpass;
		dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		if (subpass.colorAttachmentCount)
		{
			dep.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dep.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		}

		if (subpass.pDepthAttachment)
		{
			dep.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dep.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dep.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dep.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}

		for (uint32_t i = 0; i < subpass.inputAttachmentCount; i++)
		{
			// find an appropriate to input ref attachment
			auto inputRef = std::find_if(m_inputAttachmentRefs.begin(), m_inputAttachmentRefs.end(),
				[this, subpass, i](const VkAttachmentReference& ref)
				{
					return ref.attachment == m_attachmentIndices[subpass.pInputAttachments[i]];
				});

			if (inputRef != m_inputAttachmentRefs.end())
			{
				SH_ASSERT((dstSubpass != 0), "impossible to read input attachment at subpass 0");

				if (m_images[inputRef->attachment]->getImageAspect() & VK_IMAGE_ASPECT_COLOR_BIT)
				{
					dep.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					dep.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					dep.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					dep.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
				}
				else if (m_images[inputRef->attachment]->getImageAspect() & VK_IMAGE_ASPECT_DEPTH_BIT)
				{
					dep.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					dep.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					dep.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					dep.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
				}
			}
		}
		m_localDependencies.emplace_back(dep);
	}

	void RenderPass::setupExternalSubpassDependency()
	{
		auto& lastSubpass = m_subpassDescriptions.back();

		VkSubpassDependency externalDep{};
		externalDep.srcSubpass = static_cast<uint32_t>(m_subpassDescriptions.size()) - 1;
		externalDep.dstSubpass = VK_SUBPASS_EXTERNAL;
		externalDep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		if (lastSubpass.colorAttachmentCount)
		{
			externalDep.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			externalDep.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			externalDep.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			externalDep.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		}

		if (lastSubpass.pDepthStencilAttachment)
		{
			externalDep.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			externalDep.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			externalDep.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			externalDep.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}

		if (!m_config.firstRenderpass)
		{
			externalDep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			externalDep.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		// if any attachment is used as an another's renderpass input, then it's supposed to be written in the last subpass of the renderpass instance
		if (hasRenderpassInputs())
		{
			externalDep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			externalDep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		m_localDependencies.emplace_back(externalDep);
	}

	bool RenderPass::hasRenderpassInputs()
	{
		for (auto& attachment : m_attachments)
		{
			if (attachment.finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				return true;
		}
		return false;
	}

	void RenderPass::createRenderpass()
	{
		setupExternalSubpassDependency();

		if (m_subpassDescriptions.size() > 1) // TEMP?
		{
			VkSubpassDependency dep1{};
			dep1.srcSubpass = VK_SUBPASS_EXTERNAL;
			dep1.dstSubpass = m_subpassDescriptions.size() - 1;
			dep1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dep1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dep1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			m_localDependencies.emplace_back(dep1);
		}

		VkRenderPassCreateInfo renderpassInfo{};
		renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpassInfo.attachmentCount = static_cast<uint32_t>(m_clearValues.size());
		renderpassInfo.pAttachments = m_attachments.data();
		renderpassInfo.subpassCount = static_cast<uint32_t>(m_subpassDescriptions.size());
		renderpassInfo.pSubpasses = m_subpassDescriptions.data();
		renderpassInfo.dependencyCount = static_cast<uint32_t>(m_localDependencies.size());
		renderpassInfo.pDependencies = m_localDependencies.data();
		VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::getDevice()->getVkDevice(), &renderpassInfo, nullptr, &m_renderPass));

		VkSubpassDescription& lastSubpass = m_subpassDescriptions.back();
		if (m_swapchainTarget && lastSubpass.colorAttachmentCount > 1)
			delete[] lastSubpass.pColorAttachments; 
	}

	void RenderPass::setClearColor(const glm::vec4& clearColor)
	{
		for (uint8_t bit = 0; bit < m_clearValues.size(); bit++)
		{
			if (m_clearBits & 1 << bit)
				m_clearValues[bit].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		}
	}

	void RenderPass::initBeginInfo(VkRenderPassBeginInfo& info)
	{
		Device* device = VulkanContext::getDevice();
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.clearValueCount = static_cast<uint32_t>(m_clearValues.size());
		info.pClearValues = m_clearValues.data();
		info.renderPass = m_renderPass;
		info.framebuffer = m_framebuffer;
		info.renderArea.offset = { 0, 0 };
		info.renderArea.extent = { m_config.framebufferInfo.width, m_config.framebufferInfo.height };
	}

	void RenderPass::resizeFramebuffer(uint32_t width, uint32_t height)
	{
		VulkanContext::getDevice()->waitIdle();
		vkDestroyFramebuffer(VulkanContext::getDevice()->getVkDevice(), m_framebuffer, nullptr);

		m_config.framebufferInfo.width = width;
		m_config.framebufferInfo.height = height;

#ifdef SH_TRACE_FRAMEBUFFER_RESIZE
		SH_TRACE("framebuffers resized: {%u; %u}", width, height);
#endif

		for (Ref<Texture2D>& image : m_images)
		{
			if (image)
				image->resize(m_config.framebufferInfo.width, m_config.framebufferInfo.height);
		}

		createFramebuffer();

		for (size_t i = 0; i < m_subpassDescriptions.size(); i++)
			m_subpasses[i].shader->updateDescriptorSets();
	}

	void RenderPass::logRenderPassInfo() const
	{
		SH_TRACE("renderPass: {name = %s; VkRenderPass = %x}", m_name.c_str(), m_renderPass);

		for (const Ref<Texture2D>& image: m_images)
		{
			if (image)
			{
				if (image->getImageAspect() & VK_IMAGE_ASPECT_COLOR_BIT)
					SH_TRACE("color attachment: {VkImage = %x; VkImageView = %x}", image->getVkImage(), image->getImageView());
				else if (image->getImageAspect() & VK_IMAGE_ASPECT_DEPTH_BIT)
					SH_TRACE("depth attachment: {VkImage = %x; VkImageView = %x}", image->getVkImage(), image->getImageView());
			}
		}
	}

	void RenderPass::createFramebuffer()
	{
		Device* device = VulkanContext::getDevice();

		std::vector<VkImageView> imageViews(m_clearValues.size());

		for (uint32_t j = 0; j < imageViews.size(); j++)
			imageViews[j] = m_images[j]->getImageView();

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
		framebufferInfo.pAttachments = imageViews.data();
		framebufferInfo.width = m_config.framebufferInfo.width;
		framebufferInfo.height = m_config.framebufferInfo.height;
		framebufferInfo.layers = m_config.framebufferInfo.layers;
		VK_CHECK_RESULT(vkCreateFramebuffer(device->getVkDevice(), &framebufferInfo, nullptr, &m_framebuffer));
	}

	void RenderPass::setupSwapchainTarget(const SubpassAttachment& colorAttachment)
	{
		VkAttachmentDescription& swapchainTarget = m_attachments[m_clearValues.size()];
		swapchainTarget.flags = VK_DEPENDENCY_BY_REGION_BIT;
		swapchainTarget.format = VulkanContext::getDevice()->getSwapchain()->getImageFormat();
		swapchainTarget.samples = VK_SAMPLE_COUNT_1_BIT;
		swapchainTarget.loadOp = ShadowToVkCvt::attachmentLoadOpToVk(colorAttachment.loadOp);
		swapchainTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		swapchainTarget.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		swapchainTarget.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		swapchainTarget.initialLayout = swapchainTarget.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD? 
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		swapchainTarget.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // swapchain image will be used in imgui's renderPass as a color attachment

		size_t currAttachment = m_clearValues.size();
		m_attachmentRefs[currAttachment] = {static_cast<uint32_t>(currAttachment), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

		m_clearBits |= 1 << currAttachment;
		m_clearValues.emplace_back(VkClearValue{ 1.0f,0.0f });
	}

	bool RenderPass::onWidnowResized(const WindowResizedEvent& event)
	{
		uint32_t width, height;
		ShApp::get().getWindow().getFramebufferSize(width, height);
		resizeFramebuffer(width, height);
		return false;
	}

	Framebuffer::Framebuffer(const Ref<RenderPass>& renderPass, const FramebufferInfo& info)
		: m_renderPass(renderPass), m_info(info)
	{
		Device* vkDevice = VulkanContext::getDevice();
		m_framebuffers.resize(vkDevice->getSwapchain()->getImageCount());
		createFramebuffers();
	}

	Framebuffer::~Framebuffer()
	{
		clear();
	}

	bool Framebuffer::onWindowResized(const WindowResizedEvent& e)
	{
		vkQueueWaitIdle(VulkanContext::getDevice()->getGraphicsQueue());
		clear();

		m_info.width = e.width;
		m_info.height = e.height;
		SH_TRACE("framebuffers resized: {%u; %u}", e.width, e.height);

		m_renderPass->resizeFramebuffer(e.width, e.height);
		createFramebuffers();

		return false;
	}

	void Framebuffer::createFramebuffers()
	{
		m_imageViews.resize(m_renderPass->getAttachmentCount());
		Device* vkDevice = VulkanContext::getDevice();

		for (uint32_t i = 0; i < vkDevice->getSwapchain()->getImageCount(); i++)
		{
			auto& images = m_renderPass->getImages();

			for (uint32_t j = 0; j < m_imageViews.size(); j++)
				m_imageViews[j] = images[j] ? images[j]->getImageView() : vkDevice->getSwapchain()->getImageViews()[i];

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass->getVkRenderpass();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(m_imageViews.size());
			framebufferInfo.pAttachments = m_imageViews.data();
			framebufferInfo.width = m_info.width;
			framebufferInfo.height = m_info.height;
			framebufferInfo.layers = m_info.layers;
			VK_CHECK_RESULT(vkCreateFramebuffer(vkDevice->getVkDevice(), &framebufferInfo, nullptr, &m_framebuffers[i]));
		}
	}

	void Framebuffer::clear()
	{
		for (VkFramebuffer framebuffer : m_framebuffers)
			vkDestroyFramebuffer(VulkanContext::getDevice()->getVkDevice(), framebuffer, nullptr);
	}
}