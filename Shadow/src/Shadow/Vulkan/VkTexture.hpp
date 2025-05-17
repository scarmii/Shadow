#pragma once

#include "Shadow/Renderer/Texture.hpp"
#include "Shadow/Vulkan/VulkanDevice.hpp"
#include "Shadow/Vulkan/VulkanImage.hpp"
		 
#include <vulkan/vulkan.h>
#include <string>

namespace Shadow
{
	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(uint32_t width, uint32_t height, const Sampler& sampler);
		VulkanTexture2D(const std::string& imagePath, const Sampler& sampler);
		VulkanTexture2D(uint8_t* pixels, uint32_t length, const Sampler& sampler);
		VulkanTexture2D(uint32_t width, uint32_t height, AttachmentUsage usage, ImageFormat format, const Sampler& sampler = Sampler());
		virtual ~VulkanTexture2D();

		virtual void setData(void* data) override;
		virtual void resize(uint32_t newWidth, uint32_t newHeight) override;

		inline virtual uint8_t getMipLevelCount() const override { return m_mipLevels; }
		inline virtual const std::string& getPath() const override { return m_path; }

		inline const VulkanImage& getImage() const { return m_image; }
		inline const VKSampler getSampler() const { return m_sampler; }

		virtual bool operator==(const Texture2D& other) const override
		{
			return m_image.vkImage == ((VulkanTexture2D&)other).m_image.vkImage;
		}
	private:
		void createSampler(const Sampler& sampler);
	private:
		VulkanImage m_image;
		VKSampler m_sampler;
		VkFormat m_format;
		VkImageUsageFlags m_imageUsage;

		std::string m_path;
		uint32_t m_width, m_height;
		uint8_t m_mipLevels;
	};
}