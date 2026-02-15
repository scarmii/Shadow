#pragma once

#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/device.h"
#include "shadow/vulkan/texture.h"

#include <vulkan/vulkan.h>

namespace Shadow
{
	struct DrawArgs
	{
		uint32_t indexCount = 0;
		uint32_t instanceCount = 1;
		uint32_t firstVertex = 0;
		uint32_t firstIndex = 0;
		uint32_t firstInstance = 0;
		uint32_t vertexBufferOffset = 0;
		uint32_t instanceBufferOffset = 0;
	};

	class Semaphore
	{
	public:
		Semaphore(uint64_t initialValue);
		Semaphore(bool createSignaled = false);
		~Semaphore();

		inline void signal() { m_signaled = !m_signaled; }
		inline bool isSignaled() const { return m_signaled; }

		inline VkSemaphore getVkSemaphore(uint32_t frame) const { return m_semaphores[frame]; }

		static Ref<Semaphore> create(uint64_t initialValue) { return createRef<Semaphore>(initialValue); }
		static Ref<Semaphore> create(bool createSignaled = false) { return createRef<Semaphore>(createSignaled); }
	private:
		std::array<VkSemaphore, Device::s_maxFramesInFlight> m_semaphores;
		bool m_signaled = false;
		
		struct Timeline
		{
			uint64_t value;
			uint64_t upcomingValue;
		} m_timeline;
	};

	class CommandBuffer // commands should be recorded during run-time
	{
	public:
		CommandBuffer(QueueType type);
		~CommandBuffer();

		void begin();
		void end();
		void submit();
		void reset();

		void addWaitSemaphore(const Ref<Semaphore>& semaphore, PipelineStages waitStages);
		void addSignalSemaphore(const Ref<Semaphore>& semaphore, PipelineStages signalStages);
		void addWaitSemaphore(const Scope<Semaphore>& semaphore, PipelineStages waitStages);
		void addSignalSemaphore(const Scope<Semaphore>& semaphore, PipelineStages signalStages);

		void setViewport(float x, float y, float width, float height);
		void beginRenderPass(Ref<RenderPass>& renderpass);
		void endRenderPass();
		void nextSubpass();

		void setPushConstants(const void* data, uint32_t size, ShaderStage stageMask);
		void setPushConstants(const Ref<ComputePipeline>& pipe, const void* data, uint32_t size, ShaderStage stageMask);

		void drawMesh(const Mesh& mesh);
		void drawMesh(Mesh& mesh, glm::mat4& trasnform);
		void draw(uint32_t verticesCount, uint32_t firstVertex);
		void draw(const Ref<VertexBuffer>& vertexBuffer, uint32_t vertexCount = 0);
		void drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount = 0);
		void drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, const DrawArgs& args);
		void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, uint32_t instanceCount = 0);
		void drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount = 0);

		void clearColor(const Ref<Texture2D>& image, ImageLayout layout, PipelineStages srcStage, AccessFlags srcAcces, const glm::vec4 clearColor);
		void transitionImageLayout(const Ref<Texture2D>& image, ImageLayout oldLayout, ImageLayout newLayout,
			PipelineStages srcStageMask, PipelineStages dstStageMask,
			AccessFlags srcAccess, AccessFlags dstAcces); 

		void beginComputePass(const Ref<ComputePipeline>& pipe);
		void endComputePass();
		void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

		inline VkCommandBuffer getVkCommandBuffer(uint32_t frame) const { return m_cmdBuffers[frame]; }
		inline VkCommandBuffer getVkCommandBuffer() const { return m_cmdBuffers[VulkanContext::getDevice()->currentFrame()]; }
		inline const std::array<VkCommandBuffer, Device::s_maxFramesInFlight>& getVkCommandBuffers() const { return m_cmdBuffers; }

		static Ref<CommandBuffer> create(QueueType type) { return createRef<CommandBuffer>(type); }
	private:
		void createCommandBuffers(QueueType type);
		void createFences();
	private:
		QueueType m_type;
		bool m_waitImageAvailable;;

		std::array<VkCommandBuffer, Device::s_maxFramesInFlight> m_cmdBuffers;
		std::array<VkFence, Device::s_maxFramesInFlight> m_inFlightFences;

		std::array<std::vector<VkSemaphoreSubmitInfo>, Device::s_maxFramesInFlight> m_waits;
		std::array<std::vector<VkSemaphoreSubmitInfo>, Device::s_maxFramesInFlight> m_signals;

		struct DrawData
		{
			uint32_t subpass = 0;
			Ref<RenderPass> renderpass;

		} m_drawData;
	};
}