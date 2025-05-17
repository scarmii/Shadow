#pragma once

#include "Shadow/Core/Core.hpp"

namespace Shadow
{

	enum class AttachmentUsage : uint8_t
	{
		None            = 0,
		ColorAttachment = 1 << 0,
		DepthAttachment = 1 << 1,
		SubpassInput    = 1 << 2,
		RenderpassInput = 1 << 3
	};
	SH_FLAG(AttachmentUsage);

	enum class ImageFormat
	{
		None = 0,

		R8ui,
		RGB8,
		RGBA8,
		RGBA32f,

		Depth32f,
		Depth32f_Stencil8ui,
		Depth24f_Stencil8ui
	};

	struct Sampler
	{
		enum class Filter
		{
			Nearest,
			Linear
		} filter = Sampler::Filter::Linear;

		enum class AddressMode
		{
			Repeat,
			MirroredRepeat,
			ClampToEdge,
			ClampToBorder
		} addressMode = Sampler::AddressMode::Repeat;

		enum class BorderColor
		{
			Transparenti,
			Blacki,
			Whitei
		} borderColor = Sampler::BorderColor::Blacki;
	};

	class Texture2D
	{
	public:
		virtual ~Texture2D() = default;

		virtual void setData(void* data) = 0;
		virtual void resize(uint32_t newWidth, uint32_t newHeight) = 0;

		virtual uint8_t getMipLevelCount() const = 0;
		virtual const std::string& getPath() const = 0;

		static Ref<Texture2D> create(uint32_t width, uint32_t height, const Sampler& sampler = Sampler());
		static Ref<Texture2D> create(const std::string& imagePath, const Sampler& sampler = Sampler());
		static Ref<Texture2D> create(uint8_t* pixels, uint32_t length, const Sampler& sampler = Sampler());

		virtual bool operator==(const Texture2D& other) const = 0;
	};
}