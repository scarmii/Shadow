#pragma once

#include "shadow/renderer/pipelineStatesConfig.h"
#include "shadow/renderer/vertexDescription.h"
#include "shadow/vulkan/renderPass.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/texture.h"
		 
#include <vulkan/vulkan.h>

namespace Shadow
{
	class ShadowToVkCvt
	{
	public:
		static VkPrimitiveTopology primitiveTopologyToVk(PrimitiveTopology primitiveTopology);
		static VkPolygonMode polygonModeToVk(PolygonMode mode);
		static VkCullModeFlags cullModeToVk(CullMode cullMode);
		static VkFrontFace frontFaceToVk(FrontFace frontFace);
		static VkBlendFactor blendFactorToVk(BlendFactor factor);
		static VkBlendOp blendOpToVk(BlendOp op);
		static VkFormat vertexAttributeTypeToVk(VertexAttribType type);
		static VkFilter filterToVk(Sampler::Filter filter);
		static VkSamplerAddressMode addressModeToVk(Sampler::AddressMode addressMode);
		static VkBorderColor borderColorToVk(Sampler::BorderColor borderColor);
		static VkFormat imageFormatToVk(ImageFormat format);
		static VkAttachmentLoadOp attachmentLoadOpToVk(AttachmentLoadOp loadOp);
	};
}
