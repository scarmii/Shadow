#pragma once

#include "Shadow/Core/Core.hpp"
#include "Shadow/Renderer/VertexDescription.hpp"

namespace Shadow
{
	// identical to VkBufferUsageFlagBits (bitfield)
	enum class BufferUsage
	{
		None           = 0,
		UniformBuffer  = 0x00000010,
		StorageBuffer  = 0x00000020,
		IndexBuffer    = 0x00000040,
		VertexBuffer   = 0x00000080,
		IndirectBuffer = 0x00000100,
		InstanceBuffer = VertexBuffer
	};
	SH_FLAG(BufferUsage)

	class RenderBuffer
	{
	public:
		RenderBuffer(BufferUsage usage, uint32_t size, uint32_t elementCount);
		virtual ~RenderBuffer() = default;

		inline BufferUsage getUsage() const { return m_usage; }
		inline uint32_t getSize() const { return m_size; }
		inline uint32_t getElementCount() const { return m_elementCount; }

		static Ref<RenderBuffer> create(const void* data, uint32_t size, uint32_t stride, BufferUsage bufferUsage);
		static Ref<RenderBuffer> createVertexBuffer(const void* data, uint32_t size, const VertexInput& layout);
		static Ref<RenderBuffer> createVertexBuffer(const void* data, uint32_t size, uint32_t stride);
		static Ref<RenderBuffer> createIndexBuffer(uint32_t* indices, uint32_t count);
	private:
		BufferUsage m_usage;
		uint32_t m_size;
		uint32_t m_elementCount;
	};

	class ShaderBuffer : public RenderBuffer
	{
	public:
		ShaderBuffer(BufferUsage usage, uint32_t size, uint32_t elemCount);
		virtual ~ShaderBuffer() = default;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void setData_RT(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		static Ref<ShaderBuffer> createUniformBuffer(uint32_t size);
		static Ref<ShaderBuffer> createStorageBuffer(const void* data, uint32_t size, uint32_t stride, BufferUsage additionalUsage);
	};

	class VertexBuffer // can be used as vertex buffer or instance data buffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		virtual uint32_t getVertexCount() const = 0;

		static Ref<VertexBuffer> create(void* vertices, uint32_t size, const VertexInput& layout); // static buffer
		static Ref<VertexBuffer> create(void* vertices, uint32_t size, uint32_t stride); // static buffer
		static Ref<VertexBuffer> create(uint32_t size, const VertexInput& layout); // dynamic buffer
		static Ref<VertexBuffer> create(uint32_t size, uint32_t stride); // dynamic buffer
	};

	// currently Shadow supports 32-bit index buffers
	class IndexBuffer
	{
	public:
		IndexBuffer(uint32_t indexCount);
		virtual ~IndexBuffer() = default;

		inline uint32_t getCount() const { return m_indexCount; }

		static Ref<IndexBuffer> create(uint32_t* indices, uint32_t count);
	private:
		uint32_t m_indexCount;
	};

	class UniformBuffer
	{
	public:
		UniformBuffer(uint32_t size);
		virtual ~UniformBuffer() = default;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void setData_RT(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		inline uint32_t getSize() const { return m_size; }

		static Ref<UniformBuffer> create(uint32_t size);
	private:
		uint32_t m_size;
	};

	class StorageBuffer
	{
	public:
		virtual ~StorageBuffer() = default;

		virtual uint32_t getSize() const = 0;
		virtual uint32_t getElementCount() const = 0;
		virtual BufferUsage getUsage() const = 0;

		static Ref<StorageBuffer> create(const void* data, uint32_t size, uint32_t stride, BufferUsage usage);
	private:
	};
}