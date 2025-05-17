#pragma once

#include "Shadow/Renderer/Pipeline.hpp"

namespace Shadow
{
	class RenderCmdBuffer
	{
	public:
		virtual ~RenderCmdBuffer() = default;

		virtual void begin() = 0;
		virtual void end() = 0;
		virtual void submit() = 0;

		virtual void setViewport(float x, float y, float width, float height) = 0;
		virtual void beginRenderPass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants = nullptr) = 0;
		virtual void endRenderPass() = 0;
		virtual void nextSubpass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants = nullptr) = 0;

		virtual void drawMesh(const Mesh& mesh) = 0;
		virtual void draw(uint32_t verticesCount, uint32_t firstVertex = 0) = 0;
		virtual void draw(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void draw(const Ref<StorageBuffer>& vertexBuffer) = 0;
		virtual void drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount = 0) = 0;
		virtual void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, uint32_t instanceCount) = 0;
		virtual void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount) = 0;

		virtual void beginTransfer() = 0;
		virtual void submitTransfer(PipelineStages graphicsWaitStage) = 0;

		virtual void beginCompute(const Ref<ComputePipeline>& pipe, uint32_t descriptorSet, const void* pPushConstants = nullptr) = 0;
		virtual void submitCompute(PipelineStages graphicsWaitStage) = 0;
		virtual void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = 0;

		virtual void acquireFromGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess) = 0;
		virtual void releaseToGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess) = 0;
		virtual void acquireFromComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess) = 0;
		virtual void releaseToComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess) = 0;

		virtual uint32_t currentFrame() const = 0;
	};
}