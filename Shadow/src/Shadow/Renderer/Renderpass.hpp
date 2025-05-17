#pragma once

#include "Shadow/Events/EventTypes.hpp"
#include "Shadow/Renderer/Texture.hpp"

#include <vector>
#include <glm/vec4.hpp>

namespace Shadow
{
	struct FramebufferInfo
	{
		uint32_t width = 0, height = 0;
		uint32_t layers = 1, samples = 1;
	};

	struct SubpassAttachment
	{
		uint32_t ref;
		ImageFormat format;
		AttachmentUsage usage = AttachmentUsage::None;
		Sampler sampler = Sampler();
	};

	struct InputAttachment
	{
		std::string shaderName;
		uint32_t index;
	};

	struct Subpass
	{
		uint32_t colorAttachmentCount = 0;
		SubpassAttachment* pColorAttachments = nullptr;
		SubpassAttachment* pDepthAttachment = nullptr;
		uint32_t inputAttachmentCount = 0;
		uint32_t* pInputAttachmentRefs = nullptr;
	};

	struct RenderpassConfig
	{
		uint32_t subpassCount;
		Subpass* pSubpasses;
		glm::vec4 clearColor = { 0.025f,0.025f,0.025f,1.0f };
		FramebufferInfo framebufferInfo;
		bool firstRenderpass = false;
		bool swapchainTarget = false;
	};

	class Renderpass
	{
	public:
		virtual ~Renderpass() = default;

		virtual bool isSwapchainTarget() const = 0;
		virtual bool hasDepthAttachment() const = 0;

		virtual size_t getSubpassCount() const = 0;
		virtual Ref<Texture2D> getOutput(uint32_t ref) const = 0;

		static Ref<Renderpass> create(const RenderpassConfig& config);
	};
}