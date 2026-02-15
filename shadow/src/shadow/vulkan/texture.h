#pragma once

#include "shadow/core/core.h"
#include "shadow/vulkan/device.h"
		 
#include <vma/vk_mem_alloc.h>

#include <string>
#include <glm/glm.hpp>

namespace Shadow
{
	enum class ImageUsage : uint8_t
	{
		None            = 0,
		ColorAttachment = 1 << 0,
		DepthAttachment = 1 << 1,
		SubpassInput    = 1 << 2,
		SampledImage    = 1 << 3,
		StorageImage    = 1 << 4

	};
	SH_FLAG(ImageUsage);

	enum class ImageFormat : uint8_t
	{
		None = 0,

		R8ui,
		RGB8,
		RGBA8,
		RGBA32f,
		RGBA8_Unorm,

		Depth32f,
		Depth32f_Stencil8ui,
		Depth24f_Stencil8ui
	};

	struct Image
	{
		VkImage vkImage = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo{};

		~Image();
		void deallocate();
	};

	struct Sampler
	{
		enum class Filter : uint8_t
		{
			Nearest,
			Linear
		} filter = Sampler::Filter::Linear;

		enum class AddressMode : uint8_t
		{
			Repeat,
			MirroredRepeat,
			ClampToEdge,
			ClampToBorder
		} addressMode = Sampler::AddressMode::Repeat;

		enum class BorderColor :uint8_t
		{
			Transparenti,
			Blacki,
			Whitei
		} borderColor = Sampler::BorderColor::Blacki;
	};

	// are identical to VkImageLayout
	enum class ImageLayout
	{
		Undefined = 0,
		General = 1,
		ColorAttachmentOptimal = 2,
		DepthStencilAttachmentOptimal = 3,
		DepthStencilReadOnlyOptimal = 4,
		ShaderReadOnlyOptimal = 5,
		TransferSrcOptimal = 6,
		TransferDstOptimal = 7
	};

	class Texture2D
	{
	public:
		Texture2D(uint32_t width, uint32_t height, const Sampler& sampler = Sampler());
		Texture2D(const std::string& imagePath, const Sampler& sampler = Sampler());
		Texture2D(uint8_t* pixels, uint32_t length, const Sampler& sampler = Sampler());
		Texture2D(uint32_t width, uint32_t height, ImageUsage usage, ImageFormat format, const Sampler& sampler, ImageLayout initialLayout);
		Texture2D(uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat format, const Sampler& sampler = Sampler());
		~Texture2D();

		void setData(void* data);
		void resize(uint32_t width, uint32_t height);

		uint32_t getWidth() const { return m_width; }
		uint32_t getHeight() const { return m_height; }
		uint8_t getMipLevelCount() const { return m_mipLevels; }
		const std::string& getPath() const { return m_path; }

		bool isRenderpassInput() const { return m_renderpassInput; }

		inline VkImageAspectFlags getImageAspect() const { return m_imageAspect; }
		inline VkFormat getFormat() const { return m_format; }
		inline const VkImage getVkImage() const { return m_image.vkImage; }
		inline const VkImageView getImageView() const { return m_image.imageView; }
		inline const VkSampler getSampler() const { return m_sampler; }

		bool operator==(const Texture2D& other) const
		{
			return m_image.vkImage == static_cast<const Texture2D&>(other).m_image.vkImage;
		}

		static Ref<Texture2D> create(uint32_t width, uint32_t height, const Sampler& sampler = Sampler()) { return createRef<Texture2D>(width, height, sampler); }
		static Ref<Texture2D> create(const std::string& imagePath, const Sampler& sampler = Sampler()) { return createRef<Texture2D>(imagePath, sampler);}
		static Ref<Texture2D> create(uint8_t* pixels, uint32_t length, const Sampler& sampler = Sampler()) { return createRef<Texture2D>(pixels, length, sampler); }
		static Ref<Texture2D> create(uint32_t width, uint32_t height, ImageUsage usage, ImageFormat format, const Sampler& sampler = Sampler(), ImageLayout initialLayout = ImageLayout::Undefined);
		static Ref<Texture2D> create(uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat format, const Sampler& sampler = Sampler()) { return createRef<Texture2D>(width, height, usage, format, sampler); }
	private:
		void transitionToInitialImageLayout();
		void createSampler(const Sampler& sampler);
	private:
		bool m_renderpassInput = false;

		Image m_image;
		ImageLayout m_initialLayout;
		VkSampler m_sampler;
		VkFormat m_format;
		VkImageAspectFlags m_imageAspect;
		VkImageUsageFlags m_imageUsage;

		std::string m_path;
		uint32_t m_width, m_height;
		uint8_t m_mipLevels;
	};

	class Sprite2D
	{
	public:
		Sprite2D(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& tileSize, const glm::vec2& spriteSize = { 1,1 });

		const Ref<Texture2D>& getTexture() const { return m_texture; }
		const glm::vec2* getTexCoords() const { return m_texCoords; }

		static Ref<Sprite2D> create(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& tileSize, const glm::vec2& spriteSize = { 1,1 });
	private:
		Ref<Texture2D> m_texture;
		glm::vec2 m_texCoords[4];
	};
}