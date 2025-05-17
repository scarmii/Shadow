#include "shpch.hpp"
#include "ShadowToVulkanTypes.hpp"

namespace Shadow
{
	VkPrimitiveTopology ShadowToVkCvt::shadowPrimitiveTopologyToVk(PrimitiveTopology primitiveTopology)
	{
		switch (primitiveTopology)
		{
			case PrimitiveTopology::Line:	        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case PrimitiveTopology::LineStrip:      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case PrimitiveTopology::Point:	        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			case PrimitiveTopology::Triangles:	    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case PrimitiveTopology::TriangleStrip:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		}
		SH_WARN("it seems like you've passed an unknown PrimitiveTopology value :<");
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}

	VkPolygonMode ShadowToVkCvt::shadowPolygonModeToVk(PolygonMode mode)
	{
		switch (mode)
		{
			case PolygonMode::Fill:  return VK_POLYGON_MODE_FILL;
			case PolygonMode::Line:  return VK_POLYGON_MODE_LINE;
			case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
		}
		SH_WARN("it seems like you've passed an unknown PolygonMode value :<");
		return VK_POLYGON_MODE_MAX_ENUM;
	}

	VkCullModeFlags ShadowToVkCvt::shadowCullModeToVk(CullMode cullMode)
	{
		switch (cullMode)
		{
			case CullMode::Front:         return VK_CULL_MODE_FRONT_BIT;
			case CullMode::Back:          return VK_CULL_MODE_BACK_BIT;
			case CullMode::FrontAndBack:  return VK_CULL_MODE_FRONT_AND_BACK;
		}
		SH_WARN("it seems like you've passed an unknown CullMode value :<");
		return VK_CULL_MODE_NONE;
	}

	VkFrontFace ShadowToVkCvt::shadowFrontFaceToVk(FrontFace frontFace)
	{
		switch (frontFace)
		{
			case FrontFace::CounterClockwise:  return VK_FRONT_FACE_COUNTER_CLOCKWISE;
			case FrontFace::Clockwise:         return VK_FRONT_FACE_CLOCKWISE;
		}
		SH_WARN("it seems like you've passed an unknown FrontFace value :<");
		return VK_FRONT_FACE_MAX_ENUM;
	}

	VkBlendFactor ShadowToVkCvt::shadowBlendFactorToVk(BlendFactor factor)
	{
		switch (factor)
		{
			case BlendFactor::Zero:             return VK_BLEND_FACTOR_ZERO;
			case BlendFactor::One:              return VK_BLEND_FACTOR_ONE;
			case BlendFactor::SrcAlpha:         return VK_BLEND_FACTOR_SRC_ALPHA;
			case BlendFactor::DstAlpha:         return VK_BLEND_FACTOR_DST_ALPHA;
			case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		}
		SH_WARN("it seems like you've passed an unknown BlendFactor value :<");
		return VK_BLEND_FACTOR_MAX_ENUM;
	}

	VkBlendOp ShadowToVkCvt::shadowBlendOpToVk(BlendOp op)
	{
		switch (op)
		{
			case BlendOp::Add:  return VK_BLEND_OP_ADD;
		}
		SH_WARN("it seems like you've passed an unknown BlendOp value :<");
		return VK_BLEND_OP_ADD;
	}

	VkFormat ShadowToVkCvt::shadowVertexAttributeTypeToVk(VertexAttribType type)
	{
		switch (type)
		{
			case VertexAttribType::Float: return VK_FORMAT_R32_SFLOAT;
			case VertexAttribType::Vec2f: return VK_FORMAT_R32G32_SFLOAT;
			case VertexAttribType::Vec3f: return VK_FORMAT_R32G32B32_SFLOAT;
			case VertexAttribType::Vec4f: return VK_FORMAT_R32G32B32A32_SFLOAT;
	
			case VertexAttribType::Uint:  return VK_FORMAT_R32_UINT;
			case VertexAttribType::Vec2u: return VK_FORMAT_R32G32_UINT;
			case VertexAttribType::Vec3u: return VK_FORMAT_R32G32B32_UINT;
			case VertexAttribType::Vec4u: return VK_FORMAT_R32G32B32A32_UINT;

			case VertexAttribType::Int:   return VK_FORMAT_R32_SINT;
			case VertexAttribType::Vec2i: return VK_FORMAT_R32G32_SINT;
			case VertexAttribType::Vec3i: return VK_FORMAT_R32G32B32_SINT;
			case VertexAttribType::Vec4i: return VK_FORMAT_R32G32B32A32_SINT;
		}
		SH_WARN("it seems like you've passed an unknown VertexAttributeType value :<");
		return VK_FORMAT_UNDEFINED;
	}

	VkFilter ShadowToVkCvt::shadowFilterToVk(Sampler::Filter filter)
	{
		switch (filter)
		{
			case Shadow::Sampler::Filter::Nearest:  return VK_FILTER_NEAREST;
			case Shadow::Sampler::Filter::Linear:   return VK_FILTER_LINEAR;
		}
		SH_WARN("it seems like you've passed an unknown Sampler::Filter value :<");
		return VK_FILTER_MAX_ENUM;
	}

	VkSamplerAddressMode ShadowToVkCvt::shadowAddressModeToVk(Sampler::AddressMode addressMode)
	{
		switch (addressMode)
		{
			case Sampler::AddressMode::Repeat:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case Sampler::AddressMode::MirroredRepeat:  return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case Sampler::AddressMode::ClampToEdge:     return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case Sampler::AddressMode::ClampToBorder:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		}
		SH_WARN("it seems like you've passed an unknown Sampler::AddressMode value :<");
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}

	VkBorderColor ShadowToVkCvt::shadowBorderColorToVk(Sampler::BorderColor borderColor)
	{
		switch (borderColor)
		{
			case Sampler::BorderColor::Transparenti:  return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
			case Sampler::BorderColor::Blacki:        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			case Sampler::BorderColor::Whitei:        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		}
		SH_WARN("it seems like you've passed an unknown Sampler::AddressMode value :<");
		return VK_BORDER_COLOR_MAX_ENUM;
	}

	VkFormat ShadowToVkCvt::shadowImageFormatToVk(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::R8ui:                 return VK_FORMAT_R8_UINT;
			case ImageFormat::RGB8:                 return VK_FORMAT_R8G8B8_SRGB;
			case ImageFormat::RGBA8:                return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RGBA32f:              return VK_FORMAT_R32G32B32_SFLOAT;
				 
			case ImageFormat::Depth32f:             return VK_FORMAT_D32_SFLOAT;
			case ImageFormat::Depth32f_Stencil8ui:  return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case ImageFormat::Depth24f_Stencil8ui:  return VK_FORMAT_D24_UNORM_S8_UINT;
		}
		SH_WARN("it seems like you've passed an unknown ImageFormat value :<");
		return VK_FORMAT_UNDEFINED;
	}
}