#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"

#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Vulkan/VulkanDevice.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanRenderpass.hpp"
#include "Shadow/Vulkan/ShadowToVulkanTypes.hpp"
#include "Shadow/Vulkan/VulkanBuffer.hpp"
#include "Shadow/Vulkan/VkTexture.hpp"
#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"

#include <GLFW/glfw3.h>

namespace Shadow
{
	static const std::vector<const char*> s_deviceExtensions = { 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		"VK_KHR_maintenance1",
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
	};

	static std::mutex s_deviceMutex;


	static bool hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
			format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	static bool isDepthFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT|| format == VK_FORMAT_D32_SFLOAT;
	}


	VulkanDevice::VulkanDevice(const VkInstance instance, GLFWwindow* windowHandle, 
		const std::vector<const char*>& validationLayers)
		: m_vulkanInstance(instance)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		createSurface(windowHandle);
		pickPhysicalDevice();
		createLogicalDevice(validationLayers);
		createVmaAllocator();
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDeviceWaitIdle(m_vkDevice);

		delete m_swapchain;

		vmaDestroyAllocator(m_vmaAllocator);
		vkDestroyDevice(m_vkDevice, nullptr);
		vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
	}

	void VulkanDevice::init(GLFWwindow* windowHandle)
	{
		m_swapchain = new Swapchain(windowHandle, m_surface);
	}

	void VulkanDevice::allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer* buffer, VmaAllocation* allocation) const
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = memoryUsage;

		VK_CHECK_RESULT(vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, buffer, allocation, VK_NULL_HANDLE));
	}

	void VulkanDevice::allocateImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling tiling,
		VkImageUsageFlags usage, uint8_t mipLevels, VulkanImage& outImage)
	{
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = imageFormat;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo{};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		vmaallocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK_RESULT(vmaCreateImage(m_vmaAllocator, &imageInfo, &vmaallocInfo, &outImage.vkImage, &outImage.allocation, &outImage.allocationInfo));

		outImage.imageView = createImageView(outImage.vkImage, imageFormat,
			isDepthFormat(imageFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}

	VkImageView VulkanDevice::createImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, uint8_t mipLevels) const
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VK_CHECK_RESULT(vkCreateImageView(m_vkDevice, &createInfo, nullptr, &imageView));

		return imageView;
	}

	void VulkanDevice::transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format, 
		VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint8_t mipLevels) 
	{
		SH_PROFILE_FUNCTION();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = hasStencilComponent(format) ?
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		{
			SH_PROFILE_SCOPE("vkCmdPipelineBarrier - VulkanImage::transitionImageLayout")
			vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
	}

	void VulkanDevice::bufferMemoryBarrier(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize size, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t srcQueueFamily, uint32_t dstQueueFamily)
	{
		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.buffer = buffer;
		bufferBarrier.size = size;
		bufferBarrier.offset = 0;
		bufferBarrier.srcAccessMask = srcAccessMask;
		bufferBarrier.dstAccessMask = dstAccessMask;
		bufferBarrier.srcQueueFamilyIndex = srcQueueFamily;
		bufferBarrier.dstQueueFamilyIndex = dstQueueFamily;
		vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
	}

	void VulkanDevice::generateMipmaps(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format, int width, int height, uint8_t mipLevels)
	{
		SH_PROFILE_FUNCTION();

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProperties);

		if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = width, mipHeight = height;
			for (uint32_t i = 1; i < mipLevels; i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					0, 0, nullptr, 0, nullptr, 1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(cmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &barrier);

				if (mipWidth > 1)
					mipWidth /= 2;
				if (mipHeight > 1)
					mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &barrier);
		}
	}

	void VulkanDevice::copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
	{
		SH_PROFILE_FUNCTION();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };
		vkCmdCopyBufferToImage(cmdBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	void VulkanDevice::copyBufferToBuffer(VkCommandBuffer cmdBuffer, VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset)
	{
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;
		vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &copyRegion);
	}

	void VulkanDevice::pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, devices.data());

		std::unordered_map<uint8_t, uint16_t> deviceScore;
		uint16_t maxScore = 0;

		for (uint8_t i = 0; i < devices.size(); i++)
		{
			deviceScore.insert(deviceScore.cend(), { i, 0 });

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(devices[i], &props);

			VkPhysicalDeviceFeatures supportedFeatures{};
			vkGetPhysicalDeviceFeatures(devices[i], &supportedFeatures);

			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				deviceScore[i] += 400;

			if (checkDeviceExtensionSupport(devices[i]))
				deviceScore[i] += 200;

			if (supportedFeatures.samplerAnisotropy)
				deviceScore[i] += 200;

			findQueueFamilies(devices[i]);

			if (queueFamilyIndicesComplete())
				deviceScore[i] += 100;

			if (deviceScore[i] > maxScore)
				maxScore = deviceScore[i];
		}

		for (auto& score : deviceScore)
		{
			if (score.second == maxScore)
				m_physicalDevice = devices[score.first];
		}

#ifdef SH_DEBUG
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(m_physicalDevice, &props);

			SH_TRACE("selected gpu: %s", props.deviceName);
			SH_TRACE("max vertex input attributes: %u", props.limits.maxVertexInputAttributes);
			SH_TRACE("max bound descriptor sets: %u", props.limits.maxBoundDescriptorSets);
			SH_TRACE("max per stage descriptor samplers: %u", props.limits.maxPerStageDescriptorSamplers);
			SH_TRACE("max push constants size: %u", props.limits.maxPushConstantsSize);
			SH_TRACE("max compute work group count: %u", props.limits.maxComputeWorkGroupCount);
			SH_TRACE("max compute work group invocations: %u", props.limits.maxComputeWorkGroupInvocations);
			SH_TRACE("max compute work group size: %u", props.limits.maxComputeWorkGroupSize);
#endif
		SH_ASSERT((m_physicalDevice != VK_NULL_HANDLE), "failed to find a suitable gpu :<");
	}

	void VulkanDevice::createSurface(GLFWwindow* windowHandle)
	{
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_vulkanInstance, windowHandle, nullptr, &m_surface));
		SH_TRACE("created window surface!");
	}

	void VulkanDevice::createLogicalDevice(const std::vector<const char*>& validationLayers)
	{
		float queuePriority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { 
			m_graphics.presentQueue.index, m_graphics.presentQueue.index, 
			m_compute.queue.index, m_transfer.queue.index};

		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceSynchronization2Features sync2Features{};
		sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		sync2Features.synchronization2 = VK_TRUE;
		sync2Features.pNext = nullptr;

		VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
		dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
		dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
		dynamicRenderingFeatures.pNext = &sync2Features;

		VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
		timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;
		timelineSemaphoreFeatures.pNext = &dynamicRenderingFeatures;

		VkPhysicalDeviceDescriptorIndexingFeatures descriptorFeatures{};
		descriptorFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		descriptorFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		descriptorFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		descriptorFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		descriptorFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		descriptorFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		descriptorFeatures.pNext = &timelineSemaphoreFeatures;

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(s_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = s_deviceExtensions.data();
		createInfo.pNext = &descriptorFeatures;

#ifdef SH_DEBUG 
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
#elif SH_VULKAN_VALIDATION_LAYERS
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
#else
			createInfo.enabledLayerCount = 0;
#endif
		VK_CHECK_RESULT(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_vkDevice));
		SH_TRACE("created Vulkan device ^*^");

		VkQueue& graphicsQueue = m_graphics.graphicsQueue.handle;
		VkQueue& presentQueue = m_graphics.presentQueue.handle;
		VkQueue& computeQueue = m_compute.queue.handle;
		VkQueue& transferQueue = m_transfer.queue.handle;

		vkGetDeviceQueue(m_vkDevice, m_graphics.graphicsQueue.index, 0, &graphicsQueue);
		SH_TRACE("graphics queue index: %i", m_graphics.graphicsQueue.index);

		vkGetDeviceQueue(m_vkDevice, m_graphics.presentQueue.index, 0, &presentQueue);
		SH_TRACE("present queue index: %i", m_graphics.presentQueue.index);

		vkGetDeviceQueue(m_vkDevice, m_transfer.queue.index, 0, &transferQueue);
		SH_TRACE("transfer queue index: %i", m_transfer.queue.index);

		vkGetDeviceQueue(m_vkDevice, m_compute.queue.index, 0, &computeQueue);
		SH_TRACE("compute queue index: %i", m_compute.queue.index);
	}

	void VulkanDevice::createVmaAllocator()
	{
		VmaAllocatorCreateInfo allocatorCreateInfo{};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorCreateInfo.instance = VulkanContext::getVkInstance();
		allocatorCreateInfo.physicalDevice = m_physicalDevice;
		allocatorCreateInfo.device = m_vkDevice;

		VK_CHECK_RESULT(vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator));
		SH_TRACE("vmaAllocator has been created :D");
	}

	bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
		std::vector< VkExtensionProperties> availableExts(extCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, availableExts.data());

		for (const char* requiredExt : s_deviceExtensions)
		{
			if (std::find_if(availableExts.cbegin(), availableExts.cend(),
				[requiredExt](const VkExtensionProperties& ext)
				{
					return !strcmp(requiredExt, ext.extensionName);
				}) == availableExts.cend())
			{
				return false;
			}
		}
		return true;
	}

	void VulkanDevice::findQueueFamilies(VkPhysicalDevice device)
	{
		uint32_t queueFamilyCount = 0;
		VkBool32 presentSupported = false;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				m_graphics.graphicsQueue.index = i;

				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupported);

				if (presentSupported)
					m_graphics.presentQueue.index = i;
			}
			else if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
				m_compute.queue.index = i;
			else if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
				m_transfer.queue.index = i;
		}

		if (m_compute.queue.index == UINT32_MAX)
			m_compute.queue.index = m_graphics.graphicsQueue.index;
		if (m_transfer.queue.index == UINT32_MAX)
			m_transfer.queue.index = m_graphics.graphicsQueue.index;
	}

	bool VulkanDevice::queueFamilyIndicesComplete() const
	{
		return m_graphics.graphicsQueue.index != UINT32_MAX && m_graphics.presentQueue.index != UINT32_MAX 
			&& m_compute.queue.index != UINT32_MAX && m_transfer.queue.index != UINT32_MAX;
	}

	uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}

		SH_ERROR("failed to find suitable memory type");
		return -1;
	}

	VkFormat VulkanDevice::findSupportedFormat(const std::set<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features))
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features))
				return format;
		}

		SH_WARN("failed to find supported Vulkan format D:");
		return VK_FORMAT_UNDEFINED;
	}
}