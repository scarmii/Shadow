#include "shpch.hpp"
#include "Shadow/Renderer/Renderer.hpp"

#include "Shadow/Vulkan/VulkanBuffer.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"

namespace Shadow
{
	VulkanRenderBuffer::VulkanRenderBuffer(const void* data, uint32_t size, uint32_t stride, BufferUsage usage)
		: RenderBuffer(usage, size, size / stride)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkDeviceSize bufferSize = size;
		VkBufferUsageFlags bufferUsage = static_cast<VkBufferUsageFlags>(usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		device->allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, &stagingBufferAllocation);
		device->allocateBuffer(bufferSize, bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY, &m_buffer, &m_allocation);

		void* mappedData;
		vmaMapMemory(device->getVmaAllocator(), stagingBufferAllocation, &mappedData);
		memcpy(mappedData, data, size);
		vmaUnmapMemory(device->getVmaAllocator(), stagingBufferAllocation);

		Ref<VulkanCmdBuffer> cmdBuffer = as<VulkanCmdBuffer>(Renderer::getCmdBuffer());
		VkCommandBuffer vkCmdBuffer = cmdBuffer->beginSingleTimeCmdBuffer(device->getTransferQueueIndex());
		device->copyBufferToBuffer(vkCmdBuffer, stagingBuffer, m_buffer, bufferSize, 0, 0);
		cmdBuffer->submitSingleTimeCmdBuffer(vkCmdBuffer, device->getTransferQueueIndex());

		vkQueueWaitIdle(device->getTransferQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	VulkanRenderBuffer::~VulkanRenderBuffer()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		vkQueueWaitIdle(device->getTransferQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), m_buffer, m_allocation);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size, uint32_t stride)
		: m_vertexCount(size/stride)
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		VkDeviceSize bufferSize = size;

		VkBufferCreateInfo stagingBufferCI{};
		stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferCI.size = bufferSize;
		stagingBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo stagingAllocCI{};
		stagingAllocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		stagingAllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		VK_CHECK_RESULT(vmaCreateBuffer(vulkanDevice->getVmaAllocator(), &stagingBufferCI, &stagingAllocCI,
			&m_stagingBuffer.buffer, &m_stagingBuffer.allocation, &m_stagingBuffer.allocInfo));

		VkBufferCreateInfo bufferCI{};
		bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferCI.size = bufferSize;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCI{};
		allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK_RESULT(vmaCreateBuffer(vulkanDevice->getVmaAllocator(), &bufferCI, &allocCI,
			&m_vertexBuffer.buffer, &m_vertexBuffer.allocation, VK_NULL_HANDLE));
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* vertices, uint32_t size, uint32_t stride)
		: m_vertexCount(size/stride)
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		VkDeviceSize bufferSize = size;

		VkBufferCreateInfo stagingBufferCI{};
		stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferCI.size = bufferSize;
		stagingBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo stagingAllocCI{};
		stagingAllocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		stagingAllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		VK_CHECK_RESULT(vmaCreateBuffer(vulkanDevice->getVmaAllocator(), &stagingBufferCI, &stagingAllocCI,
			&m_stagingBuffer.buffer, &m_stagingBuffer.allocation, &m_stagingBuffer.allocInfo));

		memcpy(m_stagingBuffer.allocInfo.pMappedData, vertices, size);

		vulkanDevice->allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &m_vertexBuffer.buffer, &m_vertexBuffer.allocation);

		Ref<VulkanCmdBuffer> cmdBuffer = Renderer::getCmdBuffer();
		VkCommandBuffer vkCmdBuffer = cmdBuffer->beginSingleTimeCmdBuffer(vulkanDevice->getTransferQueueIndex());
		vulkanDevice->copyBufferToBuffer(vkCmdBuffer, m_stagingBuffer.buffer, m_vertexBuffer.buffer, bufferSize, 0, 0);
		cmdBuffer->submitSingleTimeCmdBuffer(vkCmdBuffer, vulkanDevice->getTransferQueueIndex());
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();

		vkQueueWaitIdle(vulkanDevice->getTransferQueue());
		vmaDestroyBuffer(vulkanDevice->getVmaAllocator(), m_vertexBuffer.buffer, m_vertexBuffer.allocation);

		if (m_stagingBuffer.buffer != VK_NULL_HANDLE)
			vmaDestroyBuffer(vulkanDevice->getVmaAllocator(), m_stagingBuffer.buffer, m_stagingBuffer.allocation);
	}

	// TODO: offsets
	void VulkanVertexBuffer::setData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(m_stagingBuffer.allocInfo.pMappedData, data, size);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = offset;
		copyRegion.dstOffset = offset;
		copyRegion.size = size;
		vkCmdCopyBuffer(as<VulkanCmdBuffer>(Renderer::getCmdBuffer())->getTransferCmdBuffer(), m_stagingBuffer.buffer, m_vertexBuffer.buffer, 1, &copyRegion);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: IndexBuffer(count)
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		VkDeviceSize bufferSize = sizeof(uint32_t) * count;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		vulkanDevice->allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, &stagingBufferAllocation);
		vulkanDevice->allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &m_buffer, &m_allocation);

		void* data;
		vmaMapMemory(VulkanContext::getVulkanDevice()->getVmaAllocator(), stagingBufferAllocation, &data);
		memcpy(data, indices, (size_t)bufferSize);
		vmaUnmapMemory(vulkanDevice->getVmaAllocator(), stagingBufferAllocation);

		Ref<VulkanCmdBuffer> cmdBuffer = Renderer::getCmdBuffer();
		VkCommandBuffer vkCmdBuffer = cmdBuffer->beginSingleTimeCmdBuffer(vulkanDevice->getTransferQueueIndex());
		vulkanDevice->copyBufferToBuffer(vkCmdBuffer, stagingBuffer, m_buffer, bufferSize, 0, 0);
		cmdBuffer->submitSingleTimeCmdBuffer(vkCmdBuffer, vulkanDevice->getTransferQueueIndex());

		vkQueueWaitIdle(vulkanDevice->getTransferQueue());
		vmaDestroyBuffer(vulkanDevice->getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		vkQueueWaitIdle(device->getGraphicsQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), m_buffer, m_allocation);
	}

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size)
		: UniformBuffer(size)
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		VkDeviceSize bufferSize = size;

		for (size_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
		{
			VkBufferCreateInfo bufferCI{};
			bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCI.size = size;
			bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocCI{};
			allocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;
			allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
			VK_CHECK_RESULT(vmaCreateBuffer(vulkanDevice->getVmaAllocator(), &bufferCI, &allocCI, &m_buffers[i], &m_allocations[i], &m_allocInfos[i]));
		}
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		vkQueueWaitIdle(device->getGraphicsQueue());

		for (size_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
			vmaDestroyBuffer(device->getVmaAllocator(), m_buffers[i], m_allocations[i]);
	}

	void VulkanUniformBuffer::setData(const void* data, uint32_t size, uint32_t offset)
	{
		for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
			memcpy(m_allocInfos[i].pMappedData, data, size);
	}

	void VulkanUniformBuffer::setData_RT(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(m_allocInfos[Renderer::getCmdBuffer()->currentFrame()].pMappedData, data, size);
	}

	VulkanStorageBuffer::VulkanStorageBuffer(const void* data, uint32_t size, uint32_t stride, BufferUsage usage)
		: m_size(size), m_elemCount(size/stride), m_usage(BufferUsage::StorageBuffer | usage)
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		VkDeviceSize bufferSize = size;
		VkBufferUsageFlags bufferUsage = static_cast<VkBufferUsageFlags>(usage) | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		vulkanDevice->allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, &stagingBufferAllocation);
		vulkanDevice->allocateBuffer(bufferSize, bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY, &m_vkBuffer, &m_allocation);

		void* mappedData;
		vmaMapMemory(vulkanDevice->getVmaAllocator(), stagingBufferAllocation, &mappedData);
		memcpy(mappedData, data, size);
		vmaUnmapMemory(vulkanDevice->getVmaAllocator(), stagingBufferAllocation);

		Ref<VulkanCmdBuffer> cmdBuffer = as<VulkanCmdBuffer>(Renderer::getCmdBuffer());
		VkCommandBuffer vkCmdBuffer = cmdBuffer->beginSingleTimeCmdBuffer(vulkanDevice->getTransferQueueIndex());
		vulkanDevice->copyBufferToBuffer(vkCmdBuffer, stagingBuffer, m_vkBuffer, bufferSize, 0, 0);
		cmdBuffer->submitSingleTimeCmdBuffer(vkCmdBuffer, vulkanDevice->getTransferQueueIndex());

		vkQueueWaitIdle(vulkanDevice->getTransferQueue());
		vmaDestroyBuffer(vulkanDevice->getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		vkQueueWaitIdle(vulkanDevice->getGraphicsQueue());
	    vmaDestroyBuffer(vulkanDevice->getVmaAllocator(), m_vkBuffer, m_allocation);
	}
}