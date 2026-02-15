#include "shpch.h"
#include "shadow/core/core.h"
#include "shadow/renderer/renderer.h"

#include "shadow/vulkan/shadowToVulkanTypes.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/texture.h"
#include "shadow/vulkan/buffer.h"
		 
#define  STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <mutex>
#include <minmax.h>

namespace Shadow
{
	static std::mutex s_imageMutex;

	SH_FLAG_DEF(ImageUsage, uint8_t);

	static bool isDepthFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT;
	}

	Image::~Image()
	{
		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(VulkanContext::getDevice()->getVkDevice(), imageView, nullptr);

		vmaDestroyImage(VulkanContext::getDevice()->getVmaAllocator(), vkImage, allocation);
	}

	void Image::deallocate() 
	{
		//vkDeviceWaitIdle(VulkanContext::getDevice()->getVkDevice());

		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(VulkanContext::getDevice()->getVkDevice(), imageView, nullptr);

		vmaDestroyImage(VulkanContext::getDevice()->getVmaAllocator(), vkImage, allocation);
	}

	Texture2D::Texture2D(uint32_t width, uint32_t height, const Sampler& sampler)
		: m_imageAspect(VK_IMAGE_ASPECT_COLOR_BIT), m_width(width), m_height(height), m_image{}
	{
		m_mipLevels = static_cast<uint8_t>(std::floor(std::log2(max(width, height)))) + 1;
		m_format = VK_FORMAT_R8G8B8A8_SRGB;
		m_imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);
			VulkanContext::getDevice()->createImage(width, height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		}

		createSampler(sampler);
	}

	Texture2D::Texture2D(const std::string& imagePath, const Sampler& sampler)
		: m_imageAspect(VK_IMAGE_ASPECT_COLOR_BIT), m_path(imagePath)
	{
		stbi_set_flip_vertically_on_load(1);
		int texChannels;
		stbi_uc* pixels = nullptr;
		{
			SH_PROFILE_SCOPE("stbi_load - VulkanTexture::VulkanTexture(const std::string& name, const std::string& imagePath, const Sampler& sampler)");
			pixels = stbi_load(imagePath.c_str(), (int*)&m_width, (int*)&m_height, &texChannels, STBI_rgb_alpha);
		}
		SH_ASSERT(pixels, "Failed to load texture image %s", imagePath.c_str());

		m_mipLevels = static_cast<uint8_t>(std::floor(std::log2(max(m_width, m_height)))) + 1;
		m_format = VK_FORMAT_R8G8B8A8_SRGB;
		m_imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);
			VulkanContext::getDevice()->createImage(m_width, m_height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		}

		setData(pixels);
		createSampler(sampler);

		stbi_image_free(pixels);
	}

	Texture2D::Texture2D(uint8_t* buffer, uint32_t length, const Sampler& sampler)
		: m_imageAspect(VK_IMAGE_ASPECT_COLOR_BIT), m_path("")
	{
		stbi_set_flip_vertically_on_load(1);
		int texChannels;
		stbi_uc* pixels = nullptr;
		{
			SH_PROFILE_SCOPE("stbi_load - VulkanTexture::VulkanTexture(uint8_t* buffer, uint32_t length, const Sampler& sampler)");
			pixels = stbi_load_from_memory(buffer, length, (int*)&m_width, (int*)&m_height, &texChannels, STBI_rgb_alpha);
		}
		SH_ASSERT(pixels, "Failed to load pixels from memory %s");

		 m_mipLevels = static_cast<uint8_t>(std::floor(std::log2(max(m_width, m_height)))) + 1;
		 m_format = VK_FORMAT_R8G8B8A8_SRGB;
		 m_imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		 {
			 std::scoped_lock<std::mutex> lock(s_imageMutex);
			 VulkanContext::getDevice()->createImage(m_width, m_height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		 }

		setData(pixels);
		createSampler(sampler);

		stbi_image_free(pixels);
	}

	Texture2D::Texture2D(uint32_t width, uint32_t height, ImageUsage usage, ImageFormat format, const Sampler& sampler, ImageLayout initialLayout)
		: m_image{}, m_initialLayout(initialLayout), m_format(ShadowToVkCvt::imageFormatToVk(format)), 
		m_imageAspect(isDepthFormat(m_format)? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT), m_width(width), m_height(height), m_mipLevels(1)
	{
		m_imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (usage & ImageUsage::SampledImage)
		{
			m_imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			m_renderpassInput = true;
		}
		if (usage & ImageUsage::SubpassInput)
			m_imageUsage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		if (usage & ImageUsage::ColorAttachment)
			m_imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (usage & ImageUsage::DepthAttachment)
			m_imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (usage & ImageUsage::StorageImage)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(VulkanContext::getDevice()->getPhysicalDevice(), ShadowToVkCvt::imageFormatToVk(format), &formatProperties);
			SH_ASSERT((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT), 
				"uncompatible format: specified image format can't be applied to create storage images");
			m_imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);
			VulkanContext::getDevice()->createImage(width, height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		}

		if (initialLayout != ImageLayout::Undefined)
			transitionToInitialImageLayout();

		createSampler(sampler);
	}

	Texture2D::Texture2D(uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat format, const Sampler& sampler)
		: m_image{}, m_format(format), m_imageAspect(isDepthFormat(m_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
		m_width(width), m_height(height), m_mipLevels(1), m_imageUsage(usage)
	{
		m_imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
			m_renderpassInput = true;

		if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(VulkanContext::getDevice()->getPhysicalDevice(), format, &formatProperties);
			SH_ASSERT((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT), "...");
		}

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);
			VulkanContext::getDevice()->createImage(width, height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		}

		createSampler(sampler);
	}

	Texture2D::~Texture2D()
	{
		vkDeviceWaitIdle(VulkanContext::getDevice()->getVkDevice());
		vkDestroySampler(VulkanContext::getDevice()->getVkDevice(), m_sampler, nullptr);
	}

	void Texture2D::setData(void* pixels)
	{
		Device* device = VulkanContext::getDevice();
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_width * m_height * 4);

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
		device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, &stagingBufferAllocation);

		void* data;
		vmaMapMemory(device->getVmaAllocator(), stagingBufferAllocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(device->getVmaAllocator(), stagingBufferAllocation);

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);

			VkCommandBuffer vkCmdBuffer = device->beginSingleTimeCmdBuffer(QueueType::Graphics);

			device->transitionImageLayout(vkCmdBuffer, m_image.vkImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0, VK_ACCESS_TRANSFER_WRITE_BIT, m_mipLevels);

			device->copyBufferToImage(vkCmdBuffer, stagingBuffer, m_image.vkImage, m_width, m_height);

			// transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
			device->generateMipmaps(vkCmdBuffer, m_image.vkImage, VK_FORMAT_R8G8B8A8_SRGB, m_width, m_height, m_mipLevels);
			device->submitSingleTimeCmdBuffer(vkCmdBuffer, QueueType::Graphics);
		}

		vkQueueWaitIdle(device->getGraphicsQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	void Texture2D::resize(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;

		VkImage image = m_image.vkImage;
		VkImageView imageView = m_image.imageView;
		VmaAllocation allocation = m_image.allocation;

		//VulkanContext::getDevice()->addToDeletionQueue([image, imageView, allocation]()
		//	{
				Device* device = VulkanContext::getDevice();
				vkDestroyImageView(device->getVkDevice(), imageView, nullptr);
				vmaDestroyImage(device->getVmaAllocator(), image, allocation);
			//});

		VulkanContext::getDevice()->createImage(width, height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);

		if (m_initialLayout != ImageLayout::Undefined)
			transitionToInitialImageLayout();
	}

	Ref<Texture2D> Texture2D::create(uint32_t width, uint32_t height, ImageUsage usage, ImageFormat format, const Sampler& sampler, ImageLayout initialLayout)
	{
		return createRef<Texture2D>(width, height, usage, format, sampler, initialLayout); 
	}

	void Texture2D::transitionToInitialImageLayout()
	{
		Device* device = VulkanContext::getDevice();
		VkCommandBuffer cmdBuffer = device->beginSingleTimeCmdBuffer(QueueType::Graphics);

		switch (m_initialLayout)
		{
			case ImageLayout::ColorAttachmentOptimal:
			{
				device->transitionImageLayout(cmdBuffer, m_image.vkImage, m_format,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, m_mipLevels);
				break;
			}
			case ImageLayout::DepthStencilAttachmentOptimal:
			{
				device->transitionImageLayout(cmdBuffer, m_image.vkImage, m_format,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, m_mipLevels);
				break;
			}
			case ImageLayout::ShaderReadOnlyOptimal:
			{
				device->transitionImageLayout(cmdBuffer, m_image.vkImage, m_format,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					0, VK_ACCESS_SHADER_READ_BIT, m_mipLevels);
				break;
			}
			case ImageLayout::General:
			{
				device->transitionImageLayout(cmdBuffer, m_image.vkImage, m_format,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
					0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, m_mipLevels);
				break;
			}
		}

		device->submitSingleTimeCmdBuffer(cmdBuffer, QueueType::Graphics);
	}

	void Texture2D::createSampler(const Sampler& sampler)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(VulkanContext::getDevice()->getPhysicalDevice(), &properties);

		VkFilter filter = ShadowToVkCvt::filterToVk(sampler.filter);
		VkSamplerAddressMode addressMode = ShadowToVkCvt::addressModeToVk(sampler.addressMode);
		VkBorderColor borderColor = ShadowToVkCvt::borderColorToVk(sampler.borderColor);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = filter;
		samplerInfo.minFilter = filter;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.borderColor = borderColor;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(m_mipLevels);

		VK_CHECK_RESULT(vkCreateSampler(VulkanContext::getDevice()->getVkDevice(), &samplerInfo, nullptr, &m_sampler));
	}

	Sprite2D::Sprite2D(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& tileSize, const glm::vec2& spriteSize)
	{
		m_texCoords[0] = { (coords.x * tileSize.x) / texture->getWidth(), (coords.y * tileSize.y) / texture->getHeight() };
		m_texCoords[1] = { ((coords.x + spriteSize.x) * tileSize.x) / texture->getWidth(), (coords.y * tileSize.y) / texture->getHeight() };
		m_texCoords[2] = { ((coords.x + spriteSize.x) * tileSize.x) / texture->getWidth(), ((coords.y + spriteSize.y) * tileSize.y) / texture->getHeight() };
		m_texCoords[3] = { (coords.x * tileSize.x) / texture->getWidth(), ((coords.y + spriteSize.y) * tileSize.y) / texture->getHeight() };
	}

	Ref<Sprite2D> Sprite2D::create(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& tileSize, const glm::vec2& spriteSize)
	{
		return createRef<Sprite2D>(texture, coords, tileSize, spriteSize);
	}
}