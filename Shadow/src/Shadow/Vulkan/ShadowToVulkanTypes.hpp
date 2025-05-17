#pragma once

#include "Shadow/Renderer/PipelineStatesConfig.hpp"
#include "Shadow/Renderer/Renderpass.hpp"
#include "Shadow/Renderer/VertexDescription.hpp"
#include "Shadow/Renderer/Pipeline.hpp"
#include "Shadow/Renderer/Texture.hpp"
		 
#include <vulkan/vulkan.h>

namespace Shadow
{
	class ShadowToVkCvt
	{
	public:
		static VkPrimitiveTopology shadowPrimitiveTopologyToVk(PrimitiveTopology primitiveTopology);
		static VkPolygonMode shadowPolygonModeToVk(PolygonMode mode);
		static VkCullModeFlags shadowCullModeToVk(CullMode cullMode);
		static VkFrontFace shadowFrontFaceToVk(FrontFace frontFace);
		static VkBlendFactor shadowBlendFactorToVk(BlendFactor factor);
		static VkBlendOp shadowBlendOpToVk(BlendOp op);
		static VkFormat shadowVertexAttributeTypeToVk(VertexAttribType type);
		static VkFilter shadowFilterToVk(Sampler::Filter filter);
		static VkSamplerAddressMode shadowAddressModeToVk(Sampler::AddressMode addressMode);
		static VkBorderColor shadowBorderColorToVk(Sampler::BorderColor borderColor);
		static VkFormat shadowImageFormatToVk(ImageFormat format);
	};
}
