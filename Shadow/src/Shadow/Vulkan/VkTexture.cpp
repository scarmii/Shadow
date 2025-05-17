#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"
#include "Shadow/Renderer/Renderer.hpp"

#include "Shadow/Vulkan/ShadowToVulkanTypes.hpp"
#include "Shadow/Vulkan/VkTexture.hpp"
#include "Shadow/Vulkan/VulkanBuffer.hpp"
#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"
		 
#define  STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <mutex>
#include <minmax.h>

namespace Shadow
{
	static std::mutex s_imageMutex;

	static bool isDepthFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT;
	}

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, const Sampler& sampler)
		: m_width(width), m_height(height), m_image{}
	{
		m_mipLevels = static_cast<uint8_t>(std::floor(std::log2(max(width, height)))) + 1;
		m_format = VK_FORMAT_R8G8B8A8_SRGB;
		m_imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);
			VulkanContext::getVulkanDevice()->allocateImage(width, height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		}

		createSampler(sampler);
	}

	VulkanTexture2D::VulkanTexture2D(const std::string& imagePath, const Sampler& sampler)
		: m_path(imagePath)
	{
		stbi_set_flip_vertically_on_load(1);
		int texChannels;
		stbi_uc* pixels = nullptr;
		{
			SH_PROFILE_SCOPE("stbi_load - VulkanTexture::VulkanTexture(const std::string& name, const std::string& imagePath, const Sampler& sampler)");
			pixels = stbi_load(imagePath.c_str(), (int*)&m_width, (int*)&m_height, &texChannels, STBI_rgb_alpha);
		}
		SH_ASSERT(pixels, "Failed to load texture image %s :<", imagePath.c_str());

		m_mipLevels = static_cast<uint8_t>(std::floor(std::log2(max(m_width, m_height)))) + 1;
		m_format = VK_FORMAT_R8G8B8A8_SRGB;
		m_imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);
			VulkanContext::getVulkanDevice()->allocateImage(m_width, m_height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		}

		setData(pixels);
		createSampler(sampler);

		stbi_image_free(pixels);
	}

	VulkanTexture2D::VulkanTexture2D(uint8_t* buffer, uint32_t length, const Sampler& sampler)
		: m_path("")
	{
		stbi_set_flip_vertically_on_load(1);
		int texChannels;
		stbi_uc* pixels = nullptr;
		{
			SH_PROFILE_SCOPE("stbi_load - VulkanTexture::VulkanTexture(uint8_t* buffer, uint32_t length, const Sampler& sampler)");
			pixels = stbi_load_from_memory(buffer, length, (int*)&m_width, (int*)&m_height, &texChannels, STBI_rgb_alpha);
		}
		SH_ASSERT(pixels, "Failed to load pixels from memory %s :<");

		 m_mipLevels = static_cast<uint8_t>(std::floor(std::log2(max(m_width, m_height)))) + 1;
		 m_format = VK_FORMAT_R8G8B8A8_SRGB;
		 m_imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		 {
			 std::scoped_lock<std::mutex> lock(s_imageMutex);
			 VulkanContext::getVulkanDevice()->allocateImage(m_width, m_height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		 }

		setData(pixels);
		createSampler(sampler);

		stbi_image_free(pixels);
	}

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, AttachmentUsage usage, ImageFormat format, const Sampler& sampler)
		: m_image{}, m_format(ShadowToVkCvt::shadowImageFormatToVk(format)), m_width(width), m_height(height), m_mipLevels(1)
	{
		m_imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (usage & AttachmentUsage::RenderpassInput)
			m_imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (usage & AttachmentUsage::SubpassInput)
			m_imageUsage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		if (usage & AttachmentUsage::ColorAttachment)
			m_imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (usage & AttachmentUsage::DepthAttachment)
			m_imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);
			VulkanContext::getVulkanDevice()->allocateImage(width, height, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
		}

		createSampler(sampler);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		vkDeviceWaitIdle(VulkanContext::getVulkanDevice()->getVkDevice());
		vkDestroySampler(VulkanContext::getVulkanDevice()->getVkDevice(), m_sampler, nullptr);
	}

	void VulkanTexture2D::setData(void* pixels)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_width * m_height * 4);

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
		device->allocateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, &stagingBufferAllocation);

		void* data;
		vmaMapMemory(device->getVmaAllocator(), stagingBufferAllocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(device->getVmaAllocator(), stagingBufferAllocation);

		{
			std::scoped_lock<std::mutex> lock(s_imageMutex);

			Ref<VulkanCmdBuffer> cmdBuffer = as<VulkanCmdBuffer>(Renderer::getCmdBuffer());
			VkCommandBuffer vkCmdBuffer = cmdBuffer->beginSingleTimeCmdBuffer(device->getGraphicsQueueIndex());

			device->transitionImageLayout(vkCmdBuffer, m_image.vkImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0, VK_ACCESS_TRANSFER_WRITE_BIT, m_mipLevels);

			device->copyBufferToImage(vkCmdBuffer, stagingBuffer, m_image.vkImage, m_width, m_height);

			// transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
			device->generateMipmaps(vkCmdBuffer, m_image.vkImage, VK_FORMAT_R8G8B8A8_SRGB, m_width, m_height, m_mipLevels);
			cmdBuffer->submitSingleTimeCmdBuffer(vkCmdBuffer, device->getGraphicsQueueIndex());
		}

		vkQueueWaitIdle(device->getGraphicsQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	void VulkanTexture2D::resize(uint32_t newWidth, uint32_t newHeight)
	{
		m_width = newWidth;
		m_height = newHeight;

		m_image.deallocate();
		VulkanContext::getVulkanDevice()->allocateImage(newWidth, newHeight, m_format, VK_IMAGE_TILING_OPTIMAL, m_imageUsage, m_mipLevels, m_image);
	}

	void VulkanTexture2D::createSampler(const Sampler& sampler)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(VulkanContext::getVulkanDevice()->getPhysicalDevice(), &properties);

		VkFilter filter = ShadowToVkCvt::shadowFilterToVk(sampler.filter);
		VkSamplerAddressMode addressMode = ShadowToVkCvt::shadowAddressModeToVk(sampler.addressMode);
		VkBorderColor borderColor = ShadowToVkCvt::shadowBorderColorToVk(sampler.borderColor);

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

		VK_CHECK_RESULT(vkCreateSampler(VulkanContext::getVulkanDevice()->getVkDevice(), &samplerInfo, nullptr, &m_sampler));
	}
}