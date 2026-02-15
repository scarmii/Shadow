#include "shpch.h"
#include "shadow/core/core.h"

#include "shadow/renderer/renderer.h"
#include "shadow/imGui/imguiLayer.h"

#include "shadow/vulkan/device.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/renderPass.h"
#include "shadow/vulkan/shadowToVulkanTypes.h"
#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/texture.h"
#include "shadow/vulkan/commandBuffer.h"

#include <GLFW/glfw3.h>

#define USE_IMGUI_CMD_BUFFER

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

	Device::Device(const VkInstance instance, GLFWwindow* windowHandle, 
		const std::vector<const char*>& validationLayers)
		: m_vulkanInstance(instance)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		m_deletionQueue.reserve(100);

		createSurface(windowHandle);
		pickPhysicalDevice();
		createLogicalDevice(validationLayers);
		createVmaAllocator();
		createCommandPools();
	}

	Device::~Device()
	{
		vkDeviceWaitIdle(m_vkDevice);

		delete m_swapchain;

		m_imageAvailableSemaphore.reset();
		m_renderCompleteSemaphore.reset();
		m_cmdBuffer.release();

		vkDestroyCommandPool(m_vkDevice, m_graphics.cmdPool, nullptr);
		vkDestroyCommandPool(m_vkDevice, m_compute.cmdPool, nullptr);
		vkDestroyCommandPool(m_vkDevice, m_transfer.cmdPool, nullptr);

		for (auto& fn : m_deletionQueue)
			fn();

		SH_TRACE("destroying vma allocator");
		vmaDestroyAllocator(m_vmaAllocator);

		vkDestroyDevice(m_vkDevice, nullptr);
		vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
	}

	void Device::init(GLFWwindow* windowHandle)
	{
		m_swapchain = new Swapchain(windowHandle, m_surface);

		m_imageAvailableSemaphore = createScope<Semaphore>();
		m_renderCompleteSemaphore = createScope<Semaphore>();

		m_cmdBuffer = CommandBuffer::create(QueueType::Graphics);
		m_cmdBuffer->addWaitSemaphore(m_imageAvailableSemaphore, PipelineStages::ColorAttachmentOutput);
		m_cmdBuffer->addSignalSemaphore(m_renderCompleteSemaphore, PipelineStages::ColorAttachmentOutput);
	}

	void Device::acquireSwapchainImage()
	{
		m_swapchain->acquireNextImage(m_imageAvailableSemaphore->getVkSemaphore(m_currentFrame));
	}

	void Device::queuePresent()
	{
		SH_PROFILE_FUNCTION();

		Device* device = VulkanContext::getDevice();
		const VkSwapchainKHR vkSwapchain = device->getSwapchain()->getVkSwapchain();
		uint32_t imageIndex = device->getSwapchain()->getCurrentImageIndex();
#ifdef USE_IMGUI_CMD_BUFFER
		VkSemaphore renderCompleteSemaphore = ShApp::get().getImGuiLayer()->getRenderCompleteSemaphore()->getVkSemaphore(m_currentFrame);
#else
		VkSemaphore renderCompleteSemaphore = m_graphics.renderCompleteSemaphores[m_currentFrame];
#endif
		m_swapchain->presentImage(renderCompleteSemaphore);
		m_currentFrame = (m_currentFrame + 1) % s_maxFramesInFlight;
	}

	void Device::flushDeletionQueueIfRequired()
	{;
		if (m_deletionQueue.size() >= 100)
		{
			for (auto& fn : m_deletionQueue)
				fn();

			m_deletionQueue.clear();
		}
	}

	void Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer* buffer, VmaAllocation* allocation) const
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCI{};
		allocCI.usage = memoryUsage;
		VK_CHECK_RESULT(vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocCI, buffer, allocation, VK_NULL_HANDLE));
	}

	void Device::createImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling tiling,
		VkImageUsageFlags usage, uint8_t mipLevels, Image& outImage)
	{
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);

		VkImageCreateInfo imageCI{};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.extent.width = width;
		imageCI.extent.height = height;
		imageCI.extent.depth = 1;
		imageCI.mipLevels = mipLevels;
		imageCI.arrayLayers = 1;
		imageCI.format = imageFormat;
		imageCI.tiling = tiling;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCI.usage = usage;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo{};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		vmaallocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK_RESULT(vmaCreateImage(m_vmaAllocator, &imageCI, &vmaallocInfo, &outImage.vkImage, &outImage.allocation, &outImage.allocationInfo));

		outImage.imageView = createImageView(outImage.vkImage, imageFormat,
			isDepthFormat(imageFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}

	VkCommandBuffer Device::beginSingleTimeCmdBuffer(QueueType submitQueue)
	{
		VkCommandBuffer singleSubmitCmdBuffer;
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		switch (submitQueue)
		{
			case QueueType::Graphics:  allocInfo.commandPool = m_graphics.cmdPool; break;
			case QueueType::Compute:   allocInfo.commandPool = m_compute.cmdPool; break;
			case QueueType::Transfer:  allocInfo.commandPool = m_transfer.cmdPool; break;
		}
		vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &singleSubmitCmdBuffer);

		VkCommandBufferBeginInfo beginSingleSubmitInfo{};
		beginSingleSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginSingleSubmitInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(singleSubmitCmdBuffer, &beginSingleSubmitInfo);

		return singleSubmitCmdBuffer;
	}

	void Device::submitSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer, QueueType submitQueue)
	{
		vkEndCommandBuffer(cmdBuffer);

		VkQueue queue = m_queues[submitQueue];
		VkCommandPool cmdPool = VK_NULL_HANDLE;

		switch (submitQueue)
		{
			case QueueType::Graphics:
			{
				cmdPool = m_graphics.cmdPool;
				break;
			}
			case QueueType::Compute:
			{
				cmdPool = m_compute.cmdPool;
				break;
			}
			case QueueType::Transfer: 
			{
				cmdPool = m_transfer.cmdPool;
				break;
			}
		}

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;
		submit.waitSemaphoreCount = 0;
		submit.signalSemaphoreCount = 0;
		vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(m_vkDevice, cmdPool, 1, &cmdBuffer);
	}

	bool Device::isDepthFormat(VkFormat format) const
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT;
	}

	VkImageView Device::createImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, uint8_t mipLevels) const
	{
		VkImageViewCreateInfo imageViewCI{};
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.image = image;
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCI.format = format;
		imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.subresourceRange.aspectMask = aspectFlags;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = mipLevels;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VK_CHECK_RESULT(vkCreateImageView(m_vkDevice, &imageViewCI, nullptr, &imageView));

		return imageView;
	}

	void Device::transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format, 
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

	void Device::bufferMemoryBarrier(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize size, 
		VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
		uint32_t srcQueueFamily, uint32_t dstQueueFamily)
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

	void Device::generateMipmaps(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format, int width, int height, uint8_t mipLevels)
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

	void Device::copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
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

	void Device::copyBufferToBuffer(VkCommandBuffer cmdBuffer, VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset)
	{
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;
		vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &copyRegion);
	}

	void Device::pickPhysicalDevice()
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

			SH_TRACE("selected GPU: %s", props.deviceName);
			SH_TRACE("max vertex input attributes = %u", props.limits.maxVertexInputAttributes);
			SH_TRACE("max bound descriptor sets = %u", props.limits.maxBoundDescriptorSets);
			SH_TRACE("max per stage descriptor samplers = %u", props.limits.maxPerStageDescriptorSamplers);
			SH_TRACE("max push constants size = %u", props.limits.maxPushConstantsSize);
			SH_TRACE("max compute work group count = %u", props.limits.maxComputeWorkGroupCount);
			SH_TRACE("max compute work group invocations = %u", props.limits.maxComputeWorkGroupInvocations);
			SH_TRACE("max compute work group size = %u", props.limits.maxComputeWorkGroupSize);
#endif
		SH_ASSERT((m_physicalDevice != VK_NULL_HANDLE), "failed to find a suitable GPU");
	}

	void Device::createSurface(GLFWwindow* windowHandle)
	{
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_vulkanInstance, windowHandle, nullptr, &m_surface));
		SH_TRACE("created window surface");
	}

	void Device::createLogicalDevice(const std::vector<const char*>& validationLayers)
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
		SH_TRACE("created vulkan device");

		VkQueue& graphicsQueue = m_graphics.graphicsQueue.handle;
		VkQueue& presentQueue = m_graphics.presentQueue.handle;
		VkQueue& computeQueue = m_compute.queue.handle;
		VkQueue& transferQueue = m_transfer.queue.handle;

		vkGetDeviceQueue(m_vkDevice, m_graphics.graphicsQueue.index, 0, &graphicsQueue);
		SH_TRACE("graphics queue index = %i", m_graphics.graphicsQueue.index);

		vkGetDeviceQueue(m_vkDevice, m_graphics.presentQueue.index, 0, &presentQueue);
		SH_TRACE("present queue index = %i", m_graphics.presentQueue.index);

		vkGetDeviceQueue(m_vkDevice, m_transfer.queue.index, 0, &transferQueue);
		SH_TRACE("transfer queue index = %i", m_transfer.queue.index);

		vkGetDeviceQueue(m_vkDevice, m_compute.queue.index, 0, &computeQueue);
		SH_TRACE("compute queue index = %i", m_compute.queue.index);

		m_queues[QueueType::Graphics] = m_graphics.graphicsQueue.handle;
		m_queues[QueueType::Compute] = m_compute.queue.handle;
		m_queues[QueueType::Transfer] = m_transfer.queue.handle;
	}

	void Device::createVmaAllocator()
	{
		VmaAllocatorCreateInfo allocatorCreateInfo{};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorCreateInfo.instance = VulkanContext::getVkInstance();
		allocatorCreateInfo.physicalDevice = m_physicalDevice;
		allocatorCreateInfo.device = m_vkDevice;

		VK_CHECK_RESULT(vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator));
		SH_TRACE("vmaAllocator has been created");
	}

	bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device)
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

	void Device::findQueueFamilies(VkPhysicalDevice device)
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

	bool Device::queueFamilyIndicesComplete() const
	{
		return m_graphics.graphicsQueue.index != UINT32_MAX && m_graphics.presentQueue.index != UINT32_MAX 
			&& m_compute.queue.index != UINT32_MAX && m_transfer.queue.index != UINT32_MAX;
	}

	void Device::createCommandPools()
	{
		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdPoolInfo.queueFamilyIndex = m_graphics.graphicsQueue.index;
		VK_CHECK_RESULT(vkCreateCommandPool(m_vkDevice, &cmdPoolInfo, nullptr, &m_graphics.cmdPool));

		cmdPoolInfo.queueFamilyIndex = m_transfer.queue.index;
		VK_CHECK_RESULT(vkCreateCommandPool(m_vkDevice, &cmdPoolInfo, nullptr, &m_transfer.cmdPool));

		cmdPoolInfo.queueFamilyIndex = m_compute.queue.index;
		VK_CHECK_RESULT(vkCreateCommandPool(m_vkDevice, &cmdPoolInfo, nullptr, &m_compute.cmdPool));
	}

	uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
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

	VkFormat Device::findSupportedFormat(const std::set<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
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

		SH_WARN("failed to find supported Vulkan format");
		return VK_FORMAT_UNDEFINED;
	}

	VkFormat Device::getDefaultDepthStencilFormat() const
	{
		return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}