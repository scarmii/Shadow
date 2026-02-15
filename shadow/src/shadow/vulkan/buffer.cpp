#include "shpch.h"
#include "shadow/renderer/renderer.h"

#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/commandBuffer.h"

namespace Shadow
{
	VertexBuffer::VertexBuffer(uint32_t size, uint32_t stride, bool storageBuffer)
		: m_size(size), m_vertexCount(size / stride), m_storageBuffer(storageBuffer)
	{
		Device* device = VulkanContext::getDevice();
		VkDeviceSize bufferSize = size;

		VkBufferCreateInfo stagingBufferCI{};
		stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferCI.size = bufferSize;
		stagingBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo stagingAllocCI{};
		stagingAllocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		stagingAllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_CHECK_RESULT(vmaCreateBuffer(device->getVmaAllocator(), &stagingBufferCI, &stagingAllocCI,
			&m_stagingBuffer.buffer, &m_stagingBuffer.allocation, &m_stagingBuffer.allocInfo));

		VkBufferUsageFlags vertexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (storageBuffer)
			vertexBufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		VkBufferCreateInfo vertexBufferCI{};
		vertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferCI.usage = vertexBufferUsage;
		vertexBufferCI.size = bufferSize;
		vertexBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCI{};
		allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			VK_CHECK_RESULT(vmaCreateBuffer(device->getVmaAllocator(), &vertexBufferCI, &allocCI,
				&m_vertexBuffer.buffers[i], &m_vertexBuffer.allocations[i], VK_NULL_HANDLE));
		}
	}

	VertexBuffer::VertexBuffer(void* vertices, uint32_t size, uint32_t stride, bool storageBuffer)
		: m_vertexCount(size/stride), m_size(size), m_storageBuffer(storageBuffer)
	{
		Device* device = VulkanContext::getDevice();
		VkDeviceSize bufferSize = size;

		VkBufferCreateInfo stagingBufferCI{};
		stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferCI.size = bufferSize;
		stagingBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo stagingAllocCI{};
		stagingAllocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		stagingAllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_CHECK_RESULT(vmaCreateBuffer(device->getVmaAllocator(), &stagingBufferCI, &stagingAllocCI,
			&m_stagingBuffer.buffer, &m_stagingBuffer.allocation, &m_stagingBuffer.allocInfo));

		memcpy(m_stagingBuffer.allocInfo.pMappedData, vertices, size);

		VkCommandBuffer cmdBuffer = device->beginSingleTimeCmdBuffer(QueueType::Transfer);

		VkBufferUsageFlags vertexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (storageBuffer)
			vertexBufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			device->createBuffer(bufferSize, vertexBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY, &m_vertexBuffer.buffers[i], &m_vertexBuffer.allocations[i]);
			device->copyBufferToBuffer(cmdBuffer, m_stagingBuffer.buffer, m_vertexBuffer.buffers[i], bufferSize, 0, 0);
		}
		device->submitSingleTimeCmdBuffer(cmdBuffer, QueueType::Transfer);
	}

	VertexBuffer::~VertexBuffer()
	{
		Device* device = VulkanContext::getDevice();
		vkQueueWaitIdle(device->getGraphicsQueue());

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
			vmaDestroyBuffer(device->getVmaAllocator(), m_vertexBuffer.buffers[i], m_vertexBuffer.allocations[i]);
	
		if (m_stagingBuffer.buffer != VK_NULL_HANDLE)
			vmaDestroyBuffer(device->getVmaAllocator(), m_stagingBuffer.buffer, m_stagingBuffer.allocation);
	}

	void VertexBuffer::setData(const Ref<CommandBuffer>& cmdBuffer, const void* data, uint32_t size, uint32_t offset)
	{
		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();
		memcpy(m_stagingBuffer.allocInfo.pMappedData, data, size);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = offset;
		copyRegion.dstOffset = offset;
		copyRegion.size = size;
		vkCmdCopyBuffer(cmdBuffer->getVkCommandBuffer(currentFrame), m_stagingBuffer.buffer, m_vertexBuffer.buffers[currentFrame], 1, &copyRegion);
	}

	void VertexBuffer::setData(const CommandBuffer& cmdBuffer, const void* data, uint32_t size, uint32_t offset)
	{
		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();
		memcpy(m_stagingBuffer.allocInfo.pMappedData, data, size);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = offset;
		copyRegion.dstOffset = offset;
		copyRegion.size = size;
		vkCmdCopyBuffer(cmdBuffer.getVkCommandBuffer(currentFrame), m_stagingBuffer.buffer, m_vertexBuffer.buffers[currentFrame], 1, &copyRegion);
	}

	void VertexBuffer::setData(const Ref<CommandBuffer>& cmdBuffer, const void* data, uint32_t count, const BufferRegion* pRegions)
	{
		SH_ASSERT((count <= 100), "currently not more than 100 regions are allowed :<");

		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();
		uint32_t dataSize = 0;
		VkBufferCopy copyRegions[100]{};

		for (uint32_t i = 0; i < count; i++)
		{
			copyRegions[i].srcOffset = pRegions[i].offset;
			copyRegions[i].dstOffset = pRegions[i].offset;
			copyRegions[i].size = pRegions[i].size;

			dataSize += pRegions[i].size;
		}

		memcpy(m_stagingBuffer.allocInfo.pMappedData, data, dataSize);

		vkCmdCopyBuffer(cmdBuffer->getVkCommandBuffer(currentFrame),
			m_stagingBuffer.buffer, m_vertexBuffer.buffers[currentFrame], count, copyRegions);
	}

	Ref<VertexBuffer> VertexBuffer::create(uint32_t size, uint32_t stride, bool storageBuffer)
	{
		return createRef<VertexBuffer>(size, stride, storageBuffer);
	}

	Ref<VertexBuffer> VertexBuffer::create(void* data, uint32_t size, uint32_t stride, bool storageBuffer)
	{
		return createRef<VertexBuffer>(size, stride, storageBuffer);
	}

	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count)
		: m_count(count)
	{
		Device* device = VulkanContext::getDevice();
		VkDeviceSize bufferSize = sizeof(uint32_t) * count;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, &stagingBufferAllocation);
		device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &m_buffer, &m_allocation);

		void* data;
		vmaMapMemory(device->getVmaAllocator(), stagingBufferAllocation, &data);
		memcpy(data, indices, (size_t)bufferSize);
		vmaUnmapMemory(device->getVmaAllocator(), stagingBufferAllocation);

		VkCommandBuffer vkCmdBuffer = device->beginSingleTimeCmdBuffer(QueueType::Transfer);
		device->copyBufferToBuffer(vkCmdBuffer, stagingBuffer, m_buffer, bufferSize, 0, 0);
		device->submitSingleTimeCmdBuffer(vkCmdBuffer, QueueType::Transfer);

		vkQueueWaitIdle(device->getTransferQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	IndexBuffer::~IndexBuffer()
	{
		Device* device = VulkanContext::getDevice();
		vkQueueWaitIdle(device->getGraphicsQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), m_buffer, m_allocation);
	}

	UniformBuffer::UniformBuffer(uint32_t size)
		: m_size(size)
	{
		Device* vulkanDevice = VulkanContext::getDevice();
		VkDeviceSize bufferSize = size;

		for (size_t i = 0; i < Device::s_maxFramesInFlight; i++)
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

	UniformBuffer::~UniformBuffer()
	{
		Device* device = VulkanContext::getDevice();
		vkQueueWaitIdle(device->getGraphicsQueue());

		for (size_t i = 0; i < Device::s_maxFramesInFlight; i++)
			vmaDestroyBuffer(device->getVmaAllocator(), m_buffers[i], m_allocations[i]);
	}

	void UniformBuffer::setData(const void* data, uint32_t size, uint32_t offset)
	{
		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
			memcpy(m_allocInfos[i].pMappedData, data, size);
	}

	void UniformBuffer::setData_RT(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(m_allocInfos[VulkanContext::getDevice()->currentFrame()].pMappedData, data, size);
	}

	void UniformBuffer::setData_RT(const void* data)
	{
		memcpy(m_allocInfos[VulkanContext::getDevice()->currentFrame()].pMappedData, data, m_size);
	}

	StorageBuffer::StorageBuffer(const void* data, uint32_t size, uint32_t stride)
		: m_size(size), m_count(size/stride)
	{
		Device* device = VulkanContext::getDevice();
		VkDeviceSize bufferSize = size;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, &stagingBufferAllocation);
		device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &m_buffer, &m_allocation);

		void* pData;
		vmaMapMemory(device->getVmaAllocator(), stagingBufferAllocation, &pData);
		memcpy(pData, data, (size_t)bufferSize);
		vmaUnmapMemory(device->getVmaAllocator(), stagingBufferAllocation);

		VkCommandBuffer vkCmdBuffer = device->beginSingleTimeCmdBuffer(QueueType::Transfer);
		device->copyBufferToBuffer(vkCmdBuffer, stagingBuffer, m_buffer, bufferSize, 0, 0);
		device->submitSingleTimeCmdBuffer(vkCmdBuffer, QueueType::Transfer);

		vkQueueWaitIdle(device->getTransferQueue());
		vmaDestroyBuffer(device->getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	StorageBuffer::~StorageBuffer()
	{
		vmaDestroyBuffer(VulkanContext::getDevice()->getVmaAllocator(), m_buffer, m_allocation);
	}
}