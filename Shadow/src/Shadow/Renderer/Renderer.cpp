#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"	
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"

namespace Shadow
{
	struct RendererData
	{
		ShaderLibrary shaderLib;
		Ref<RenderCmdBuffer> cmdBuffer;
	};
	static RendererData* s_data;

	void Renderer::init()
	{
		s_data = new RendererData();
		s_data->cmdBuffer = createRef<VulkanCmdBuffer>();
	}

	void Renderer::shutdown()
	{
		delete s_data;
	}

	void Renderer::begin()
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->begin();
	}

	void Renderer::end()
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->end();
	}

	void Renderer::drawMesh(const Mesh& mesh)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->drawMesh(mesh);
	}

	void Renderer::setViewport(float x, float y, float width, float height)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->setViewport(x, y, width, height);
	}

	void Renderer::beginRenderPass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->beginRenderPass(pipe, pPushConstants);
	}

	void Renderer::endRenderPass()
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->endRenderPass();
	};

	void Renderer::draw(uint32_t verticesCount, uint32_t firstVertex)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->draw(verticesCount, firstVertex);
	}

	void Renderer::nextSubpass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->nextSubpass(pipe, pPushConstants);
	}

	void Renderer::draw(const Ref<VertexBuffer>& vertexBuffer)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->draw(vertexBuffer);
	}

	void Renderer::draw(const Ref<StorageBuffer>& vertexBuffer)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->draw(vertexBuffer);
	}

	void Renderer::drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->drawIndexed(vertexBuffer, indexBuffer, indexCount);
	}

	void Renderer::drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, uint32_t instanceCount)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->drawInstanced(vertexBuffer,instanceBuffer, instanceCount);
	}

	void Renderer::drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->drawInstanced(vertexBuffer, instanceBuffer, indexBuffer, instanceCount);
	}

	void Renderer::beginTransfer()
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->beginTransfer();
	}

	void Renderer::submitTransfer(PipelineStages graphicsWaitStage)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->submitTransfer(graphicsWaitStage);
	}

	void Renderer::submitCompute(PipelineStages graphicsWaitStage)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->submitCompute(graphicsWaitStage);
	}

	void Renderer::beginCompute(const Ref<ComputePipeline>& pipe, uint32_t descriptorSet, const void* pPushConstants)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->beginCompute(pipe, descriptorSet, pPushConstants);
	}

	void Renderer::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->dispatch(groupX, groupY, groupZ);
	}

	void Renderer::memoryBarrier(PipelineStages srcStage, PipelineStages dstStage, AccessFlags srcAccess, AccessFlags dstAccess)
	{
	}

	void Renderer::graphicsToComputeBarrier(const Ref<RenderBuffer>& buffer, PipelineStages srcStageMask, PipelineStages dstStageMask, AccessFlags srcAccess, AccessFlags dstAccess)
	{
	}

	void Renderer::computeToGraphicsBarrier(const Ref<RenderBuffer>& buffer, PipelineStages srcStageMask, PipelineStages dstStageMask, AccessFlags srcAccess, AccessFlags dstAccess)
	{
	}

	ShaderLibrary& Renderer::getShaderLibrary()
	{
		return s_data->shaderLib;
	}

	const Ref<RenderCmdBuffer>& Renderer::getCmdBuffer()
	{
		return s_data->cmdBuffer;
	}

	RendererType Renderer::getRendererType()
	{
		return RendererType::Vulkan;
	}

	void Renderer::acquireFromGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->acquireFromGraphicsQueue(buffer, dstStage, dstAccess);
	}

	void Renderer::releaseToGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->releaseToGraphicsQueue(buffer, srcStage, srcAccess);
	}

	void Renderer::acquireFromComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->acquireFromGraphicsQueue(buffer, dstStage, dstAccess);
	}

	void Renderer::releaseToComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		s_data->cmdBuffer->releaseToComputeQueue(buffer, srcStage, srcAccess);
	}
}