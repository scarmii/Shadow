#pragma once

#include "shadow/vulkan/swapchain.h"
#include "shadow/vulkan/pipeline.h"

#include <vma/vk_mem_alloc.h>

struct GLFWwindow;

namespace Shadow
{
	enum class ImageFormat : uint8_t;
	struct Image;
	class CommandBuffer;
	class Semaphore;

	enum class QueueType : int8_t
	{
		None     = -1,
		Graphics =  0,
		Transfer =  1,
		Compute  =  2
	};

	class Device
	{
	public:
		Device(const VkInstance instance, GLFWwindow* windowHandle, const std::vector<const char*>& validationLayers);
		~Device();

		void init(GLFWwindow* windowHandle);

		void acquireSwapchainImage();
		void queuePresent();

		void flushDeletionQueueIfRequired();
		inline void waitIdle() const { vkDeviceWaitIdle(m_vkDevice); }

		inline void addToDeletionQueue(const std::function<void()>& fn) { m_deletionQueue.emplace_back(fn); }
		inline const std::vector<std::function<void()>>& getDeletionQueue() const { return m_deletionQueue; }

		void transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint8_t mipLevels); // moves to image class?

		void bufferMemoryBarrier(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize size,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
			uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED);

		void generateMipmaps(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format, int width, int height, uint8_t mipLevels); // moves to image class?
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, uint8_t mipLevels) const; // moves to image class?

		void copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);
		void copyBufferToBuffer(VkCommandBuffer cmdBuffer,VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		VkFormat findSupportedFormat(const std::set<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
		VkFormat getDefaultDepthStencilFormat() const;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer* buffer, VmaAllocation* allocation) const;
		void createImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling tiling, VkImageUsageFlags usage, uint8_t mipLevels, Image& outImage);

		VkCommandBuffer beginSingleTimeCmdBuffer(QueueType submitQueue);
		void submitSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer, QueueType submitQueue);

		inline bool hasDedicatedComputeQueue() const { return m_graphics.graphicsQueue.index != m_compute.queue.index; }
		inline bool hasDedicatedTransferQueue() const { return m_graphics.graphicsQueue.index != m_transfer.queue.index; }
		bool isDepthFormat(VkFormat format) const;

		inline uint32_t currentFrame() const { return m_currentFrame; }

		inline const Ref<CommandBuffer>& getCmdBuffer() const { return m_cmdBuffer; }
		inline Swapchain* getSwapchain() const { return m_swapchain; }
		inline const Scope<Semaphore>& getImageAvailableSem() const { return m_imageAvailableSemaphore; }
		inline const Scope<Semaphore>& getRenderCompleteSem() const { return m_renderCompleteSemaphore; }

		inline VkDevice getVkDevice() const { return m_vkDevice; }
		inline VmaAllocator getVmaAllocator() const { return m_vmaAllocator; }
		inline VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }

		inline uint32_t getGraphicsQueueIndex() const { return m_graphics.graphicsQueue.index; }
		inline uint32_t getPresentQueueIndex() const { return m_graphics.presentQueue.index; }
		inline uint32_t getComputeQueueIndex() const { return m_compute.queue.index; }
		inline uint32_t getTransferQueueIndex() const { return m_transfer.queue.index; }

		inline VkQueue getQueue(QueueType type) const { return m_queues[type]; }
		inline VkQueue getGraphicsQueue() const { return m_graphics.graphicsQueue.handle; }
		inline VkQueue getPresentQueue() const { return m_graphics.presentQueue.handle; }
		inline VkQueue getComputeQueue() const { return m_compute.queue.handle; }
		inline VkQueue getTransferQueue() const { return m_transfer.queue.handle; }

		inline VkCommandPool getGraphicsCmdPool() const { return m_graphics.cmdPool; }
		inline VkCommandPool getComputeCmdPool() const { return m_compute.cmdPool; }
		inline VkCommandPool getTransferCmdPool() const { return m_transfer.cmdPool; }
	public:
		static const uint32_t s_maxFramesInFlight = 2;
	private:
		void pickPhysicalDevice();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		void findQueueFamilies(VkPhysicalDevice device);
		bool queueFamilyIndicesComplete() const;

		void createSurface(GLFWwindow* windowHandle);
		void createLogicalDevice(const std::vector<const char*>& validationLayers);
		void createVmaAllocator();
		void createCommandPools();
	private:
		uint32_t m_currentFrame = 0;

		Ref<CommandBuffer> m_cmdBuffer;
		Scope<Semaphore> m_imageAvailableSemaphore;
		Scope<Semaphore> m_renderCompleteSemaphore;

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
		mutable std::unordered_map<QueueType, VkQueue> m_queues;

		// unified graphics & present queue
		struct Graphics
		{
			VulkanQueue graphicsQueue;
			VulkanQueue presentQueue;
			VkCommandPool cmdPool;
		} m_graphics;
		
		struct Transfer
		{
			VulkanQueue queue;
			VkCommandPool cmdPool;
		} m_transfer;

		struct Compute
		{
			VulkanQueue queue;
			VkCommandPool cmdPool;
		} m_compute;

		std::vector<std::function<void()>> m_deletionQueue;
	};
}