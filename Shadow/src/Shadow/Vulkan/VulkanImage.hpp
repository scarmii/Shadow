#pragma once

#include <vma/vk_mem_alloc.h>

namespace Shadow
{
	struct VulkanImage
	{
		VkImage vkImage = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo{};

		~VulkanImage();
		void deallocate();
	};
}