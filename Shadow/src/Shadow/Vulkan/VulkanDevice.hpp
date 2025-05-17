#pragma once

#include "Shadow/Vulkan/Swapchain.hpp"
#include "Shadow/Vulkan/VulkanImage.hpp"
#include "Shadow/Renderer/Pipeline.hpp"

#include <vma/vk_mem_alloc.h>

struct GLFWwindow;

namespace Shadow
{
	enum class ImageFormat;

	class VulkanDevice
	{
	public:
		VulkanDevice(const VkInstance instance, GLFWwindow* windowHandle, const std::vector<const char*>& validationLayers);
		~VulkanDevice();

		void init(GLFWwindow* windowHandle);

		inline void waitIdle() const { vkDeviceWaitIdle(m_vkDevice); }

		void transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint8_t mipLevels);

		void bufferMemoryBarrier(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize size,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
			uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED);

		void generateMipmaps(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format, int width, int height, uint8_t mipLevels);
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, uint8_t mipLevels) const;

		void copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);
		void copyBufferToBuffer(VkCommandBuffer cmdBuffer,VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		VkFormat findSupportedFormat(const std::set<VkFormat>& candidates, VkImageTiling tiling,
			VkFormatFeatureFlags features) const;

		void allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer* buffer, VmaAllocation* allocation) const;
		void allocateImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling tiling,
			VkImageUsageFlags usage, uint8_t mipLevels, VulkanImage& outImage);

		inline bool hasDedicatedComputeQueue() const { return m_graphics.graphicsQueue.index != m_compute.queue.index; }
		inline bool hasDedicatedTransferQueue() const { return m_graphics.graphicsQueue.index != m_transfer.queue.index; }

		inline VkDevice getVkDevice() const { return m_vkDevice; }
		inline VmaAllocator getVmaAllocator() const { return m_vmaAllocator; }
		inline VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
		inline Swapchain* getSwapchain() const { return m_swapchain; }

		inline uint32_t getGraphicsQueueIndex() const { return m_graphics.graphicsQueue.index; }
		inline uint32_t getPresentQueueIndex() const { return m_graphics.presentQueue.index; }
		inline uint32_t getComputeQueueIndex() const { return m_compute.queue.index; }
		inline uint32_t getTransferQueueIndex() const { return m_transfer.queue.index; }

		inline VkQueue getGraphicsQueue() const { return m_graphics.graphicsQueue.handle; }
		inline VkQueue getPresentQueue() const { return m_graphics.presentQueue.handle; }
		inline VkQueue getComputeQueue() const { return m_compute.queue.handle; }
		inline VkQueue getTransferQueue() const { return m_transfer.queue.handle; }

		inline const uint32_t maxFramesInFlight() const { return s_maxFramesInFlight; }
	public:
		static const uint32_t s_maxFramesInFlight = 2;
	private:
		void pickPhysicalDevice();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		void createSurface(GLFWwindow* windowHandle);
		void createLogicalDevice(const std::vector<const char*>& validationLayers);
		void createVmaAllocator();

		void findQueueFamilies(VkPhysicalDevice device);
		bool queueFamilyIndicesComplete() const;
	private:
		uint32_t m_currentFrame = 0;

		const VkInstance m_vulkanInstance;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_vkDevice;
		VmaAllocator m_vmaAllocator;

		VkSurfaceKHR m_surface;
		Swapchain* m_swapchain;

		struct VulkanQueue
		{
			uint32_t index = UINT32_MAX;
			VkQueue handle = VK_NULL_HANDLE;
		};

		// unified graphics & present queue
		struct Graphics
		{
			VulkanQueue graphicsQueue;
			VulkanQueue presentQueue;
		} m_graphics;
		
		struct Transfer
		{
			VulkanQueue queue;
		} m_transfer;

		struct Compute
		{
			VulkanQueue queue;
		} m_compute;
	};
}