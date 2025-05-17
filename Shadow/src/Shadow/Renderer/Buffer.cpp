#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"
#include "Shadow/Renderer/Buffer.hpp"
#include "Shadow/Renderer/Renderer.hpp"
        
#include "Shadow/Vulkan/VulkanBuffer.hpp"

namespace Shadow
{
    SH_FLAG_DEF(BufferUsage, uint32_t);

    RenderBuffer::RenderBuffer(BufferUsage usage, uint32_t size, uint32_t elementCount)
        : m_usage(usage), m_size(size), m_elementCount(elementCount)
    {
    }

    Ref<RenderBuffer> RenderBuffer::create(const void* data, uint32_t size, uint32_t stride, BufferUsage bufferUsage)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanRenderBuffer>(data, size, stride, bufferUsage);
        }
        SH_ASSERT(false, "failed to create vertex buffer :(");
        return nullptr;
    }

    Ref<RenderBuffer> RenderBuffer::createVertexBuffer(const void* data, uint32_t size, const VertexInput& layout)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanRenderBuffer>(data, size, layout.getStride(), BufferUsage::VertexBuffer);
        }
        SH_ASSERT(false, "failed to create vertex buffer :(");
        return nullptr;
    }

    Ref<RenderBuffer> RenderBuffer::createVertexBuffer(const void* data, uint32_t size, uint32_t stride)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanRenderBuffer>(data, size, stride, BufferUsage::VertexBuffer);
        }
        SH_ASSERT(false, "failed to create vertex buffer :(");
        return nullptr;
    }

    Ref<RenderBuffer> RenderBuffer::createIndexBuffer(uint32_t* indices, uint32_t count)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanRenderBuffer>(indices, sizeof(uint32_t) * count, sizeof(uint32_t), BufferUsage::IndexBuffer);
        }
        SH_ASSERT(false, "failed to create a buffer :(");
        return nullptr;
    }


    ShaderBuffer::ShaderBuffer(BufferUsage usage, uint32_t size, uint32_t elemCount)
        : RenderBuffer(usage, size, elemCount)
    {
    }

    Ref<ShaderBuffer> ShaderBuffer::createUniformBuffer(uint32_t size)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            //case RendererType::Vulkan: return createRef<VulkanUniformBuffer>(size);
        }
        SH_ASSERT(false, "failed to create a buffer :(");
        return nullptr;
    }

    Ref<ShaderBuffer> ShaderBuffer::createStorageBuffer(const void* data, uint32_t size, uint32_t stride, BufferUsage additionalUsage)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            //case RendererType::Vulkan: return createRef<VulkanStorageBuffer>(data, size, stride, BufferUsage::StorageBuffer | additionalUsage);
        }
        SH_ASSERT(false, "failed to create a buffer :(");
        return nullptr;
    }

    Ref<VertexBuffer> VertexBuffer::create(uint32_t size, const VertexInput& layout)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanVertexBuffer>(size, layout.getStride());
        }
        SH_ASSERT(false, "failed to create vertex buffer :(");
        return nullptr;
    }

    Ref<VertexBuffer> VertexBuffer::create(void* vertices, uint32_t size, const VertexInput& layout)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanVertexBuffer>(vertices, size, layout.getStride());
        }
        SH_ASSERT(false, "failed to create vertex buffer :(");
        return nullptr;
    }

    Ref<VertexBuffer> VertexBuffer::create(uint32_t size, uint32_t stride)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanVertexBuffer>(size, stride);
        }
        SH_ASSERT(false, "failed to create vertex buffer :(");
        return nullptr;
    }

    Ref<VertexBuffer> VertexBuffer::create(void* vertices, uint32_t size, uint32_t stride)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan: return createRef<VulkanVertexBuffer>(vertices, size, stride);
        }
        SH_ASSERT(false, "failed to create vertex buffer :(");
        return nullptr;
    }

    IndexBuffer::IndexBuffer(uint32_t indexCount)
        : m_indexCount(indexCount)
    {
    }

    Ref<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t count)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None: return nullptr;
            case RendererType::Vulkan:  return createRef<VulkanIndexBuffer>(indices, count);
        }

        SH_ASSERT(false, "failed to create index buffer :(");
        return nullptr;
    }

    UniformBuffer::UniformBuffer(uint32_t size)
        : m_size(size)
    {
    }

    Ref<UniformBuffer> UniformBuffer::create(uint32_t size)
    {
        switch (Renderer::getRendererType())
        {
        	case RendererType::None:   return nullptr;
        	case RendererType::Vulkan: return createRef<VulkanUniformBuffer>(size);
        }
        SH_ASSERT(false, "failed to create uniform buffer: it seems like you're using unknown renderer API :(");
        return nullptr;
    }

    Ref<StorageBuffer> StorageBuffer::create(const void* data, uint32_t size, uint32_t stride, BufferUsage usage)
    {
        switch (Renderer::getRendererType())
        {
        	case RendererType::None:   return nullptr;
        	case RendererType::Vulkan: return createRef<VulkanStorageBuffer>(data, size, stride, usage);
        }
        SH_ASSERT(false, "failed to create storage buffer: it seems like you're using unknown renderer API :(");
        return nullptr;
    }
}