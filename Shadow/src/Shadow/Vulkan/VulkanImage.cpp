#include "shpch.hpp"
#include "Shadow/Vulkan/VulkanImage.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"

namespace Shadow
{
	VulkanImage::~VulkanImage()
	{
		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(VulkanContext::getVulkanDevice()->getVkDevice(), imageView, nullptr);

		vmaDestroyImage(VulkanContext::getVulkanDevice()->getVmaAllocator(), vkImage, allocation);
	}

	void VulkanImage::deallocate()
	{
		vkDeviceWaitIdle(VulkanContext::getVulkanDevice()->getVkDevice());

		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(VulkanContext::getVulkanDevice()->getVkDevice(), imageView, nullptr);

		vmaDestroyImage(VulkanContext::getVulkanDevice()->getVmaAllocator(), vkImage, allocation);
	}
}