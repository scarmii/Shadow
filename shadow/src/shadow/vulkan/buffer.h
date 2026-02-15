#pragma once

#include "shadow/vulkan/device.h"
		 
#include <vma/vk_mem_alloc.h>

namespace Shadow
{
	class CommandBuffer;

	struct BufferRegion
	{
		uint32_t offset;
		uint32_t size;
	};

	class VertexBuffer
	{
	public:
		VertexBuffer(uint32_t size, uint32_t stride, bool storageBuffer = false);
		VertexBuffer(void* data, uint32_t size, uint32_t stride, bool storageBuffer = false);
		~VertexBuffer();

		void setData(const Ref<CommandBuffer>& cmdBuffer, const void* data, uint32_t size, uint32_t offset = 0);
		void setData(const CommandBuffer& cmdBuffer, const void* data, uint32_t size, uint32_t offset = 0);
		void setData(const Ref<CommandBuffer>& cmdBuffer, const void* data, uint32_t count, const BufferRegion* pRegions);

		inline uint32_t getSize() const { return m_size; }
		inline uint32_t getVertexCount() const { return m_vertexCount; }
		inline bool isStorageBuffer() const { return m_storageBuffer; }
		inline const VkBuffer getVkBuffer(uint32_t currentFrame) const { return m_vertexBuffer.buffers[currentFrame]; }
		inline const VkBuffer getStagingBuffer() const { return m_stagingBuffer.buffer; }

		static Ref<VertexBuffer> create(uint32_t size, uint32_t stride, bool storageBuffer = false);
		static Ref<VertexBuffer> create(void* data, uint32_t size, uint32_t stride, bool storageBuffer = false);
	private:
		uint32_t m_size;
		uint32_t m_vertexCount;
		bool m_storageBuffer;

		struct
		{
			std::array<VkBuffer, Device::s_maxFramesInFlight> buffers;
			std::array<VmaAllocation, Device::s_maxFramesInFlight> allocations;
		} m_vertexBuffer;

		struct
		{
			VkBuffer buffer;
			VmaAllocation allocation;
			VmaAllocationInfo allocInfo;
		} m_stagingBuffer;
	};

	class IndexBuffer
	{
	public:
		IndexBuffer(uint32_t* indices, uint32_t count);
		~IndexBuffer();

		inline uint32_t getCount() const { return m_count; }
		inline const VkBuffer getVkBuffer() const { return m_buffer; }

		static Ref<IndexBuffer> create(uint32_t* indices, uint32_t count) { return createRef<IndexBuffer>(indices, count); }
	private:
		uint32_t m_count;
		VkBuffer m_buffer;
		VmaAllocation m_allocation;
	};

	class UniformBuffer
	{
	public:
		UniformBuffer(uint32_t size);
		~UniformBuffer();

		void setData(const void* data, uint32_t size, uint32_t offset = 0);
		void setData_RT(const void* data, uint32_t size, uint32_t offset = 0);
		void setData_RT(const void* data);

		inline uint32_t getSize() const { return m_size; }
		inline const std::array<VkBuffer, Device::s_maxFramesInFlight>& getBuffers() const { return m_buffers; }
		inline const VkBuffer const getVkBuffer(uint16_t index) const { return m_buffers[index]; }

		static Ref<UniformBuffer> create(uint32_t size) { return createRef<UniformBuffer>(size); }
	private:
		uint32_t m_size;
		std::array<VkBuffer, Device::s_maxFramesInFlight> m_buffers;
		std::array<VmaAllocation, Device::s_maxFramesInFlight> m_allocations;
		std::array<VmaAllocationInfo, Device::s_maxFramesInFlight> m_allocInfos;
	};

	class StorageBuffer
	{
	public:
		StorageBuffer(const void* data, uint32_t size, uint32_t stride);
		~StorageBuffer();

		inline uint32_t getSize() const { return m_size; }
		inline uint32_t getCount() const { return m_count; }
		inline const VkBuffer getVkBuffer() const { return m_buffer; }

		static Ref<StorageBuffer> create(const void* data, uint32_t size, uint32_t stride) { return createRef<StorageBuffer>(data, size, stride); }
	private:
		uint32_t m_size;
		uint32_t m_count;

		VkBuffer m_buffer;
		VmaAllocation m_allocation;
	};
}