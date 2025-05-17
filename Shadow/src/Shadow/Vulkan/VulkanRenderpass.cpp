#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"
#include "Shadow/Core/ShEngine.hpp"
#include "Shadow/Renderer/Renderer.hpp"

#include "Shadow/Vulkan/VulkanRenderpass.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanImage.hpp"
#include "Shadow/Vulkan/VulkanShader.hpp"
#include "Shadow/Vulkan/ShadowToVulkanTypes.hpp"
		 
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

namespace Shadow
{
	VulkanRenderpass::VulkanRenderpass(const RenderpassConfig& config)
		: m_config(config)
	{
		EventDispatcher::get().addReciever(SH_CALLBACK(VulkanRenderpass::onResized));

		for (uint32_t i = 0; i < config.subpassCount; i++)
			addSubpass(config.pSubpasses[i]);

		createRenderpass();
		createFramebuffers();

		setClearColor(config.clearColor);
	}

	VulkanRenderpass::~VulkanRenderpass()
	{
		vkDeviceWaitIdle(VulkanContext::getVulkanDevice()->getVkDevice());

		clear();
		vkDestroyRenderPass(VulkanContext::getVulkanDevice()->getVkDevice(), m_renderpass, nullptr);
	}

	void VulkanRenderpass::addSubpass(const Subpass& subpass)
	{
		VkAttachmentReference* pColorRefs = nullptr;

		if (subpass.colorAttachmentCount > 0)
			pColorRefs = m_attachmentRefs.data() + subpass.pColorAttachments->ref;

		VkAttachmentReference* pInputRefs = subpass.inputAttachmentCount > 0 ?
			m_inputRefs.data() + *subpass.pInputAttachmentRefs : nullptr;

		VkAttachmentReference* pDepthRef = subpass.pDepthAttachment ?
			m_attachmentRefs.data() + subpass.pDepthAttachment->ref : nullptr;

		AttachmentUsage subpassFlags = AttachmentUsage::None;

		for (uint32_t i = 0; i < subpass.colorAttachmentCount; i++)
		{
			VkAttachmentDescription& colorAttachment = m_attachments[subpass.pColorAttachments[i].ref];
			colorAttachment.format = ShadowToVkCvt::shadowImageFormatToVk(subpass.pColorAttachments[i].format);
			colorAttachment.flags = VK_DEPENDENCY_BY_REGION_BIT;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = subpass.pColorAttachments[i].usage & AttachmentUsage::RenderpassInput ?
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			m_images[subpass.pColorAttachments[i].ref] = createRef<VulkanTexture2D>(m_config.framebufferInfo.width, m_config.framebufferInfo.height,
				subpass.pColorAttachments[i].usage, subpass.pColorAttachments[i].format, subpass.pColorAttachments[i].sampler);

			pColorRefs[i] = VkAttachmentReference{ subpass.pColorAttachments[i].ref, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			m_flags |= AttachmentUsage::ColorAttachment;

			if (subpass.pColorAttachments[i].usage & AttachmentUsage::SubpassInput)
				m_inputRefs[subpass.pColorAttachments[i].ref] = VkAttachmentReference{ subpass.pColorAttachments[i].ref, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}

		if (subpass.pDepthAttachment)
		{
			VkFormat requestedFormat = ShadowToVkCvt::shadowImageFormatToVk(subpass.pDepthAttachment->format);
			VkFormat retrievedFormat = VulkanContext::getVulkanDevice()->findSupportedFormat(
				{ requestedFormat, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);

			VkAttachmentDescription& depthAttachment = m_attachments[subpass.pDepthAttachment->ref];
			depthAttachment.flags = VK_DEPENDENCY_BY_REGION_BIT;
			depthAttachment.format = retrievedFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = subpass.pDepthAttachment->usage & AttachmentUsage::RenderpassInput ?
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			*pDepthRef = { subpass.pDepthAttachment->ref, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			m_images[subpass.pDepthAttachment->ref] = createRef<VulkanTexture2D>(m_config.framebufferInfo.width, m_config.framebufferInfo.height,
				subpass.pDepthAttachment->usage, subpass.pDepthAttachment->format, subpass.pDepthAttachment->sampler);

			m_flags |= AttachmentUsage::DepthAttachment;

			if (subpass.pDepthAttachment->usage & AttachmentUsage::SubpassInput)
				m_inputRefs[subpass.pDepthAttachment->ref] = VkAttachmentReference{ subpass.pDepthAttachment->ref, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}

		VkSubpassDescription subpassDescription{};
		subpassDescription.flags = 0;
		subpassDescription.colorAttachmentCount = subpass.colorAttachmentCount;
		subpassDescription.pColorAttachments = pColorRefs;
		subpassDescription.pDepthStencilAttachment = subpass.pDepthAttachment ? pDepthRef : nullptr;
		subpassDescription.inputAttachmentCount = subpass.inputAttachmentCount;
		subpassDescription.pInputAttachments = pInputRefs;
		m_subpassDescriptions.emplace_back(subpassDescription);

		setupSubpassDependency(subpass);
	}

	void VulkanRenderpass::setupSubpassDependency(const Subpass& subpass)
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
			auto inputRef = std::find_if(m_inputRefs.begin(), m_inputRefs.end(),
				[subpass, i](const VkAttachmentReference& ref)
				{
					return ref.attachment == subpass.pInputAttachmentRefs[i];
				});

			if (inputRef != m_inputRefs.end())
			{
				SH_ASSERT((dstSubpass != 0), "impossible to read input attachment at subpass 0");

				if (m_attachments[inputRef->attachment].finalLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				{
					dep.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					dep.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					dep.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					dep.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
				}
				else if (m_attachments[inputRef->attachment].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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

	void VulkanRenderpass::setupExternalSubpassDependency()
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

	bool VulkanRenderpass::hasRenderpassInputs()
	{
		for (auto& attachment : m_attachments)
		{
			if (attachment.finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				return true;
		}
		return false;
	}

	void VulkanRenderpass::createRenderpass()
	{
		initClearValues();

		if (m_config.swapchainTarget)
			setupSwapchainTargetSubpass();

		setupExternalSubpassDependency();

		VkRenderPassCreateInfo renderpassInfo{};
		renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpassInfo.attachmentCount = static_cast<uint32_t>(m_clearValues.size());
		renderpassInfo.pAttachments = m_attachments.data();
		renderpassInfo.subpassCount = static_cast<uint32_t>(m_subpassDescriptions.size());
		renderpassInfo.pSubpasses = m_subpassDescriptions.data();
		renderpassInfo.dependencyCount = static_cast<uint32_t>(m_localDependencies.size());
		renderpassInfo.pDependencies = m_localDependencies.data();
		VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::getVulkanDevice()->getVkDevice(), &renderpassInfo, nullptr, &m_renderpass));

		VkSubpassDescription& lastSubpass = m_subpassDescriptions.back();

		if (m_config.swapchainTarget && lastSubpass.colorAttachmentCount > 1)
			delete[] lastSubpass.pColorAttachments; 
	}

	void VulkanRenderpass::setClearColor(const glm::vec4& clearColor)
	{
		for (uint8_t bit = 0; bit < m_clearValues.size(); bit++)
		{
			if (m_clearBits & 1 << bit)
				m_clearValues[bit].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		}
	}

	void VulkanRenderpass::initBeginInfo(VkRenderPassBeginInfo& info)
	{
		VulkanDevice* pVkDevice = VulkanContext::getVulkanDevice();

		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.clearValueCount = static_cast<uint32_t>(m_clearValues.size());
		info.pClearValues = m_clearValues.data();
		info.renderPass = m_renderpass;
		info.framebuffer = m_framebuffers[pVkDevice->getSwapchain()->getCurrentImageIndex()];
		info.renderArea.offset = { 0, 0 };
		info.renderArea.extent = pVkDevice->getSwapchain()->getExtent();
	}

	void VulkanRenderpass::initClearValues()
	{
		// set clear bits (depth attachment -> 0; color attachment -> 1)
		uint32_t attachmentCount = 0;

		for (uint32_t i = 0; i < m_attachments.size(); i++)
		{
			VkAttachmentReference* pAttachmentRef = m_attachmentRefs.data() + i;

			if (pAttachmentRef->layout == VK_IMAGE_LAYOUT_UNDEFINED)
				break;

			if (pAttachmentRef->layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				m_clearBits |= 1 << i;

			attachmentCount++;
		}
		m_clearValues.insert(m_clearValues.begin(), attachmentCount, { 1.0, 0 });
	}

	void VulkanRenderpass::clear()
	{
		for (VkFramebuffer framebuffer : m_framebuffers)
			vkDestroyFramebuffer(VulkanContext::getVulkanDevice()->getVkDevice(), framebuffer, nullptr);
	}

	void VulkanRenderpass::createFramebuffers()
	{
		VulkanDevice* vkDevice = VulkanContext::getVulkanDevice();

		for (size_t i = 0; i < vkDevice->getSwapchain()->getImageCount(); i++)
		{
			std::vector<VkImageView> imageViews(m_clearValues.size());

			for (uint32_t j = 0; j < imageViews.size(); j++)
			{
				imageViews[j] = j == m_clearValues.size() - 1 && m_config.swapchainTarget ?
					vkDevice->getSwapchain()->getImageViews()[i] : m_images[j]->getImage().imageView;
			}

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderpass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
			framebufferInfo.pAttachments = imageViews.data();
			framebufferInfo.width = m_config.framebufferInfo.width;
			framebufferInfo.height = m_config.framebufferInfo.height;
			framebufferInfo.layers = m_config.framebufferInfo.layers;
			VK_CHECK_RESULT(vkCreateFramebuffer(vkDevice->getVkDevice(), &framebufferInfo, nullptr, &m_framebuffers[i]));
		}
	}

	void VulkanRenderpass::setupSwapchainTargetSubpass()
	{
		uint32_t swapchainTargetIndex = m_clearValues.size();
		int32_t dstSubpass = m_subpassDescriptions.size() - 1;

		VkAttachmentDescription lastPass{};
		lastPass.format = VulkanContext::getVulkanDevice()->getSwapchain()->getImageFormat();
		lastPass.samples = VK_SAMPLE_COUNT_1_BIT;
		lastPass.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lastPass.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		lastPass.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		lastPass.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		lastPass.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		lastPass.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // imgui renderpass is responsible for transition image layout to VK_PRESENT_SRC_KHR 
		lastPass.flags = VK_DEPENDENCY_BY_REGION_BIT;

		m_attachments[swapchainTargetIndex] = std::move(lastPass);

		VkAttachmentReference* pColorRef = m_attachmentRefs.data() + swapchainTargetIndex;
		*pColorRef = { swapchainTargetIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		if (dstSubpass < 0)
		{
			VkSubpassDescription subpass{};
			subpass.flags = 0;
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = pColorRef;
			subpass.inputAttachmentCount = 0;
			subpass.pDepthStencilAttachment = nullptr;
			subpass.preserveAttachmentCount = 0;
			m_subpassDescriptions.emplace_back(std::move(subpass));

			VkSubpassDependency dep{};
			dep.srcSubpass = VK_SUBPASS_EXTERNAL;
			dep.dstSubpass = 0;
			dep.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dep.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			m_localDependencies.emplace_back(dep);
		}
		else
		{
			VkSubpassDescription& lastSubpass = m_subpassDescriptions.back();
			VkSubpassDependency& dep = m_localDependencies.back();
			VkAttachmentReference* refs = new VkAttachmentReference[5]; // this allocation will be freed in VulkanRenderpass::createRenderpass

			for (uint32_t i = 0; i < lastSubpass.colorAttachmentCount; i++)
				refs[i] = lastSubpass.pColorAttachments[i];

			refs[lastSubpass.colorAttachmentCount] = *pColorRef;

			lastSubpass.colorAttachmentCount += 1;
			lastSubpass.pColorAttachments = refs;

			VkSubpassDependency extDep{};
			extDep.srcSubpass = VK_SUBPASS_EXTERNAL;
			extDep.dstSubpass = dstSubpass;
			extDep.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			extDep.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			extDep.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			extDep.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			m_localDependencies.emplace_back(extDep);
		}

		m_clearBits |= 1 << swapchainTargetIndex;
		m_clearValues.emplace_back(VkClearValue{ 1.0, 0.0 });
	}

	bool VulkanRenderpass::onResized(const WindowResizedEvent& event)
	{
		VulkanContext::getVulkanDevice()->waitIdle();
		clear();

		ShEngine::get().getWindow().getFramebufferSize(m_config.framebufferInfo.width, m_config.framebufferInfo.height);

		for (Ref<VulkanTexture2D>& image : m_images)
		{
			if (image)
				image->resize(m_config.framebufferInfo.width, m_config.framebufferInfo.height);
		}

		createFramebuffers();

		return false;
	}
}