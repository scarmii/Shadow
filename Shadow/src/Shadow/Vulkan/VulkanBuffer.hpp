#pragma once

#include "Shadow/Renderer/Buffer.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
		 
#include <vma/vk_mem_alloc.h>

namespace Shadow
{
	class VulkanRenderBuffer : public RenderBuffer
	{
	public:
		VulkanRenderBuffer(const void* data, uint32_t size, uint32_t stride, BufferUsage bufferUsage);
		virtual ~VulkanRenderBuffer();

		inline const VkBuffer getVkBuffer() const { return m_buffer; }
	private:
		VkBuffer m_buffer;
		VmaAllocation m_allocation;
	};

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(uint32_t size, uint32_t stride);
		VulkanVertexBuffer(void* data, uint32_t size, uint32_t stride);
		virtual ~VulkanVertexBuffer();

		virtual void setData(const void* data, uint32_t size, uint32_t offset) override;

		virtual uint32_t getVertexCount() const { return m_vertexCount; }

		inline const VkBuffer getVkBuffer() const { return m_vertexBuffer.buffer; }
	private:
		uint32_t m_vertexCount;

		struct
		{
			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;
		} m_vertexBuffer;

		struct
		{
			VkBuffer buffer;
			VmaAllocation allocation;
			VmaAllocationInfo allocInfo;
		} m_stagingBuffer;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~VulkanIndexBuffer();

		inline const VkBuffer getVkBuffer() const { return m_buffer; }
	private:
		VkBuffer m_buffer;
		VmaAllocation m_allocation;
	};

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32_t size);
		virtual ~VulkanUniformBuffer();

		virtual void setData(const void* data, uint32_t size, uint32_t offset) override;
		virtual void setData_RT(const void* data, uint32_t size, uint32_t offset) override;

		inline const std::array<VkBuffer, VulkanDevice::s_maxFramesInFlight>& getBuffers() const { return m_buffers; }
		inline const VkBuffer const getVkBuffer(uint16_t index) const { return m_buffers[index]; }
	private:
		std::array<VkBuffer, VulkanDevice::s_maxFramesInFlight> m_buffers;
		std::array<VmaAllocation, VulkanDevice::s_maxFramesInFlight> m_allocations;
		std::array<VmaAllocationInfo, VulkanDevice::s_maxFramesInFlight> m_allocInfos;
	};

	class VulkanStorageBuffer : public StorageBuffer
	{
	public:
		VulkanStorageBuffer(const void* data, uint32_t size, uint32_t stride, BufferUsage usage);
		virtual ~VulkanStorageBuffer();

		virtual uint32_t getSize() const { return m_size; }
		virtual uint32_t getElementCount() const { return m_elemCount; }
		virtual BufferUsage getUsage() const { return m_usage; }

		inline const VkBuffer getVkBuffer() const { return m_vkBuffer; }
	private:
		uint32_t m_size;
		uint32_t m_elemCount;
		BufferUsage m_usage;

		VkBuffer m_vkBuffer;
		VmaAllocation m_allocation;
	};
}