#pragma once
 
#include "shadow/events/eventTypes.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/shader.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/texture.h"

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>

namespace Shadow
{
	class Shader;

	enum class AttachmentLoadOp : uint8_t
	{
		None = 0,
		Clear = 1,
		Load = 2,
		DontCare = 3
	};

	struct FramebufferInfo
	{
		uint32_t width = 0, height = 0;
		uint32_t layers = 1, samples = 1;
	};

	struct SubpassAttachment
	{
		// if swapchainTarget = true -> format, imageUsage and sampler fields are ignored
		std::string name;
		ImageFormat format;
		AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
		ImageUsage imageUsage = ImageUsage::None;
		Sampler sampler = Sampler(); // 
		ImageLayout initialLayout = ImageLayout::Undefined;
		ImageLayout finalLayout = ImageLayout::Undefined;
		bool swapchainTarget = false;
	};

	struct Subpass
	{
		Ref<GraphicsPipeline> pipeline;
		Ref<Shader> shader;
		uint32_t colorAttachmentCount = 0;
		SubpassAttachment* pColorAttachments = nullptr;
		SubpassAttachment* pDepthAttachment = nullptr;
		uint32_t inputAttachmentCount = 0;
		std::string* pInputAttachments = nullptr;
	};

	struct RenderPassConfig
	{
		uint32_t subpassCount;
		Subpass* pSubpasses;
		glm::vec4 clearColor = { 0.025f,0.025f,0.025f,1.0f };
		FramebufferInfo framebufferInfo;
		bool firstRenderpass = false;
	};

	class RenderPass
	{
	public:
		RenderPass(const RenderPassConfig& config, const std::string& name = "");
		~RenderPass();

		void initBeginInfo(VkRenderPassBeginInfo& info);
		void resizeFramebuffer(uint32_t width, uint32_t height);
		void logRenderPassInfo() const;

		inline bool isSwapchainTarget() const { return m_swapchainTarget; }
		inline bool hasDepthAttachment() const { return m_hasDepthAttachment; }

		inline size_t getSubpassCount() const { return m_config.subpassCount; }
		inline const Ref<Texture2D>& getImage(uint32_t ref) const { return m_images[ref]; }
		inline const Ref<Texture2D>& getImage(const std::string& name) const { return m_images[m_attachmentIndices[name]]; }
		inline const std::string& getName() const { return m_name; }

		inline const FramebufferInfo& getFramebufferInfo() const { return m_config.framebufferInfo; }
		inline uint32_t getFramebufferWidth() const { return m_config.framebufferInfo.width; }
		inline uint32_t getFramebufferHeight() const { return m_config.framebufferInfo.height; }

		inline const Ref<GraphicsPipeline>& getGraphicsPipeline(uint32_t subpass) const { return m_subpasses[subpass].pipeline; }
		inline const Ref<Shader>& getShader(uint32_t subpass) const { return m_subpasses[subpass].shader; }
		inline const std::array<Ref<Texture2D>, 16>& getImages() const { return m_images; }
		inline uint32_t getAttachmentCount() const { return static_cast<uint32_t>(m_clearValues.size()); }
		inline uint32_t getAttachmentIndex(const std::string& name) const { return m_attachmentIndices[name]; }
		inline const VkAttachmentDescription& getAttachmentDescription(const std::string& name) const { return m_attachments[m_attachmentIndices[name]]; }

		inline const VkRenderPass getVkRenderpass() const { return m_renderPass; }
		inline const VkImage getVkImage(uint32_t index) const { return m_images[index]->getVkImage(); }
		inline const VkImageView getImageView(uint32_t index) const { return m_images[index]->getImageView(); }

		static Ref<RenderPass> create(const RenderPassConfig& config, const std::string& name = "") { return createRef<RenderPass>(config, name); }
	private:
		bool onWidnowResized(const WindowResizedEvent& event);
		bool hasRenderpassInputs();

		void addSubpass(const Subpass& subpass, uint32_t index);
		void setupSubpassDependency(const Subpass& subpass);
		void setupExternalSubpassDependency();
		void setupSwapchainTarget(const SubpassAttachment& colorAttachment);
		void setClearColor(const glm::vec4& clearColor);

		void createRenderpass();
		void createFramebuffer();
	private:
		bool m_swapchainTarget = false;
		bool m_hasDepthAttachment = false;
		RenderPassConfig m_config;
		std::string m_name;
		mutable std::unordered_map<std::string, uint32_t> m_attachmentIndices;

		struct SubpassData
		{
			Ref<GraphicsPipeline> pipeline;
			Ref<Shader> shader;
		};
		std::array<SubpassData, 5> m_subpasses;

		VkRenderPass m_renderPass;

		uint8_t m_clearBits = 0;	// 0 - depth attachment bit; 1 - color attachment bit
		std::vector<VkClearValue> m_clearValues;

		VkFramebuffer m_framebuffer;
		std::array<Ref<Texture2D>, 16> m_images{};

		std::array<VkAttachmentDescription, 16> m_attachments{};
		std::array<VkAttachmentReference, 16> m_attachmentRefs{};
		std::array<VkAttachmentReference, 16> m_inputAttachmentRefs{};

		std::vector<VkSubpassDescription> m_subpassDescriptions;
		std::vector<VkSubpassDependency> m_localDependencies;
	};

	class Framebuffer
	{
	public:
		Framebuffer(const Ref<RenderPass>& renderPass, const FramebufferInfo& info);
		~Framebuffer();
	private:
		bool onWindowResized(const WindowResizedEvent& e);
		void createFramebuffers();
		void clear();
	private:
		Ref<RenderPass> m_renderPass;
		FramebufferInfo m_info;

		std::vector<VkFramebuffer> m_framebuffers;
		std::vector<VkImageView> m_imageViews;
	};
}