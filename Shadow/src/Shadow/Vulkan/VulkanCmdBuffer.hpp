#pragma once

#include "Shadow/Renderer/RenderCmdBuffer.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"

namespace Shadow
{
	class VulkanCmdBuffer : public RenderCmdBuffer
	{
	public:
		VulkanCmdBuffer();
		virtual ~VulkanCmdBuffer();

		virtual void begin() override;
		virtual void end() override;
		virtual void submit() override;

		virtual void setViewport(float x, float y, float width, float height) override;
		virtual void beginRenderPass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants) override;
		virtual void endRenderPass() override;
		virtual void nextSubpass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants) override;

		virtual void drawMesh(const Mesh& mesh) override;
		virtual void draw(uint32_t verticesCount, uint32_t firstVertex) override;
		virtual void draw(const Ref<VertexBuffer>& vertexBuffer) override;
		virtual void draw(const Ref<StorageBuffer>& vertexBuffer) override;
		virtual void drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount = 0) override;
		virtual void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, uint32_t instanceCount) override;
		virtual void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount) override;

		virtual void beginTransfer() override;
		virtual void submitTransfer(PipelineStages graphicsWaitStage) override;

		virtual void beginCompute(const Ref<ComputePipeline>& pipe, uint32_t descriptorSet, const void* pPushConstants) override;
		virtual void submitCompute(PipelineStages graphicsWaitStage) override;
		virtual void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) override;

		virtual void acquireFromGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess) override;
		virtual void releaseToGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess) override;
		virtual void acquireFromComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess) override;
		virtual void releaseToComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess) override;

		virtual uint32_t currentFrame() const override { return m_currentFrame; }

		void queuePresent();

		void syncWithTransferQueue(PipelineStages waitStage, PipelineStages signalStage);
		void syncWithComputeQueue(PipelineStages waitStage, PipelineStages signalStage);
		void addGraphicsQueueSubmitInfo(const VkSubmitInfo2& info);

		VkCommandBuffer beginSingleTimeCmdBuffer(uint32_t submitQueueIndex);
		void submitSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer, uint32_t submitQueueIndex);

		inline VkCommandPool getGraphicsCmdPool() const { return m_graphics.cmdPool; }
		inline VkCommandPool getComputeCmdPool() const { return m_compute.cmdPool; }

		inline VkCommandBuffer getGraphicsCmdBuffer() const { return m_graphics.cmdBuffers[m_currentFrame]; }
		inline VkCommandBuffer getComputeCmdBuffer() const { return m_compute.cmdBuffers[m_currentFrame]; }
		inline VkCommandBuffer getTransferCmdBuffer() const { return m_transfer.cmdBuffers[m_currentFrame]; }

		inline VkSemaphore getRenderCompleteSemaphore() const { return m_graphics.signalSemaphores[m_currentFrame][0]; }
		inline VkFence getInFlightFence() const { return m_graphics.inFlightFences[m_currentFrame]; }
		inline VkSemaphore getComputeCompleteSemaphore() const { return m_compute.completeSemaphores[m_currentFrame]; }
		inline VkSemaphore getComputeReadySemaphores() const { return m_compute.readySemaphores[m_currentFrame]; }
	private:
		void createCmdBuffers();
		void createCmdBufferPools();
		void createSyncObjects();
	private:
		uint32_t m_currentFrame = 0;

		struct Graphics
		{
			VkCommandPool cmdPool;
			std::array<VkCommandBuffer, VulkanDevice::s_maxFramesInFlight> cmdBuffers;

			std::array<VkSemaphore, VulkanDevice::s_maxFramesInFlight> imageAvailableSemaphores;
			std::array<VkSemaphore, VulkanDevice::s_maxFramesInFlight> renderCompleteSemaphores;
			std::array<VkFence, VulkanDevice::s_maxFramesInFlight> inFlightFences;

			bool syncWithTransferQueue = false, syncWithComputeQueue = false;
			std::array<std::vector<VkSemaphore>, VulkanDevice::s_maxFramesInFlight> signalSemaphores;
			std::array<std::vector<VkSemaphore>, VulkanDevice::s_maxFramesInFlight> waitSemaphores;
			std::vector<VkPipelineStageFlags> signalStages;
			std::vector<VkPipelineStageFlags> waitStages;

			std::array<VkSubmitInfo2, 2> submitInfos;
		} m_graphics;

		struct Transfer
		{
			VkCommandPool cmdPool;
			std::array<VkCommandBuffer, VulkanDevice::s_maxFramesInFlight> cmdBuffers;

			std::array<VkSemaphore, VulkanDevice::s_maxFramesInFlight> semaphores;
			std::array<VkFence, VulkanDevice::s_maxFramesInFlight> inFlightFences;
		} m_transfer;

		struct Compute
		{
			VkCommandPool cmdPool;
			std::array<VkCommandBuffer, VulkanDevice::s_maxFramesInFlight> cmdBuffers;

			std::array<VkSemaphore, VulkanDevice::s_maxFramesInFlight> readySemaphores;
			std::array<VkSemaphore, VulkanDevice::s_maxFramesInFlight> completeSemaphores;
			std::array<VkFence, VulkanDevice::s_maxFramesInFlight> inFlightFences;
		} m_compute;
	};
}