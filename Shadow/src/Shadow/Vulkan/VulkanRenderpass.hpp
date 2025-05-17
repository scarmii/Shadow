#pragma once
 
#include "Shadow/Events/EventTypes.hpp"

#include "Shadow/Renderer/Renderpass.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VkTexture.hpp"

#include <vector>
#include <vulkan/vulkan.h>

namespace Shadow
{
	class VulkanShader;

	class VulkanRenderpass : public Renderpass
	{
	public:
		VulkanRenderpass(const RenderpassConfig& config);
		virtual ~VulkanRenderpass();

		void initBeginInfo(VkRenderPassBeginInfo& info);

		virtual bool isSwapchainTarget() const override { return m_config.swapchainTarget; }
		virtual bool hasDepthAttachment() const override { return m_flags & AttachmentUsage::DepthAttachment; }

		virtual size_t getSubpassCount() const override { return m_config.subpassCount; }
		virtual Ref<Texture2D> getOutput(uint32_t ref) const override { return m_images[ref]; }
	
		inline const VkRenderPass getVkRenderpass() const { return m_renderpass; }
		inline const VulkanImage& getVulkanImage(uint32_t index) const { return m_images[index]->getImage(); }
	private:
		bool onResized(const WindowResizedEvent& event);

		void addSubpass(const Subpass& subpass);
		void setupSubpassDependency(const Subpass& subpass);
		void setupExternalSubpassDependency();

		bool hasRenderpassInputs();

		void createRenderpass();
		void createFramebuffers();
		void setupSwapchainTargetSubpass();

		void initClearValues();
		void setClearColor(const glm::vec4& clearColor);
		void clear();
	private:
		RenderpassConfig m_config;

		AttachmentUsage m_flags = AttachmentUsage::None;
		VkRenderPass m_renderpass;

		uint8_t m_clearBits = 0;	// 0 - depth attachment bit; 1 - color attachment bit
		std::vector<VkClearValue> m_clearValues;

		std::array<VkFramebuffer, 3> m_framebuffers{};
		std::array<Ref<VulkanTexture2D>, 5> m_images{};

		std::array<VkAttachmentDescription,5> m_attachments{};
		std::array<VkAttachmentReference, 5> m_attachmentRefs{};
		std::array<VkAttachmentReference, 5> m_inputRefs{};

		std::vector<VkSubpassDescription> m_subpassDescriptions;
		std::vector<VkSubpassDependency> m_localDependencies;
	};
}