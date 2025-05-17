#pragma once

#include "Shadow/Renderer/Mesh.hpp"
#include "Shadow/Renderer/Pipeline.hpp"
#include "Shadow/Renderer/Buffer.hpp"
#include "Shadow/Renderer/UniformBuffer.hpp"
#include "Shadow/Renderer/Shader.hpp"
#include "Shadow/Renderer/Camera.hpp"
#include "Shadow/Renderer/RenderCmdBuffer.hpp"

struct GLFWwindow;

namespace Shadow
{
	enum class RendererType
	{
		None,
		Vulkan
	};

	class Renderer
	{
	public:
		static void init();
		static void shutdown();

		static void begin();
		static void end();

		static void setViewport(float x, float y, float width, float height);
		static void beginRenderPass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants = nullptr);
		static void endRenderPass();
		static void nextSubpass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants = nullptr);

		static void drawMesh(const Mesh& mesh);
		static void draw(const Ref<VertexBuffer>& vertexBuffer);
		static void draw(const Ref<StorageBuffer>& vertexBuffer);
		static void draw(const Ref<RenderBuffer>& vertexBuffer);
		static void draw(uint32_t verticesCount, uint32_t firstVertex);
		static void drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount = 0);
		static void drawIndexed(const Ref<RenderBuffer>& vertexBuffer, const Ref<RenderBuffer>& indexBuffer, uint32_t indexCount = 0);
		static void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, uint32_t instanceCount = 0);
		static void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount = 0);
		static void drawInstanced(const Ref<RenderBuffer>& vertexBuffer, const Ref<RenderBuffer>& instanceBuffer,
			const Ref<RenderBuffer>& indexBuffer, uint32_t instanceCount = 0);

		static void beginTransfer();
		static void submitTransfer(PipelineStages graphicsWaitStage);

		static void beginCompute(const Ref<ComputePipeline>& pipe, uint32_t descriptorSet, const void* pPushConstants = nullptr);
		static void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ);
		static void submitCompute(PipelineStages graphicsWaitStage);

		static void acquireFromGraphicsQueue(const Ref<RenderBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess);
		static void acquireFromGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess);
		static void releaseToGraphicsQueue(const Ref<RenderBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess);
		static void releaseToGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess);
		static void acquireFromComputeQueue(const Ref<RenderBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess);
		static void acquireFromComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess);
		static void releaseToComputeQueue(const Ref<RenderBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess);
		static void releaseToComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess);

		// these functions currently aren't supposed to be used  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		static void memoryBarrier(PipelineStages srcStageMask, PipelineStages dstStageMask, AccessFlags srcAccess, AccessFlags dstAccess);
		static void graphicsToComputeBarrier(const Ref<RenderBuffer>& buffer, PipelineStages srcStageMask, PipelineStages dstStageMask, AccessFlags srcAccess, AccessFlags dstAccess);
		static void computeToGraphicsBarrier(const Ref<RenderBuffer>& buffer, PipelineStages srcStageMask, PipelineStages dstStageMask, AccessFlags srcAccess, AccessFlags dstAccess);
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		static ShaderLibrary& getShaderLibrary();
		static const Ref<RenderCmdBuffer>& getCmdBuffer();
		static RendererType getRendererType();
	};
}