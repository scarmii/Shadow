#pragma once

#include "shadow/events/eventDispatcher.h"
#include "shadow/vulkan/renderPass.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/commandBuffer.h"

#include <imgui/backends/imgui_impl_vulkan.h>

#include <unordered_map>

namespace Shadow
{
	struct ImGuiRenderPass
	{
		VkRenderPass vkHandle = VK_NULL_HANDLE;
		Ref<GraphicsPipeline> pipeline = nullptr;
	};

	class ImGuiLayer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void begin();
		void end();
		void updateWindows();
		void blockEvents(bool block) { m_blockEvents = block; }

		ImTextureID addTexture(const Ref<Texture2D>& texture);
		void removeTexture(ImTextureID id);

		inline const Scope<CommandBuffer>& getCommandBuffer() const { return m_imGuiCmdBuffer; }
		inline const Scope<Semaphore>& getRenderCompleteSemaphore() const { return m_imGuiRenderCompleteSemaphore; }
	private:
		bool onWindowResized(const WindowResizedEvent& e);
		bool onMouseScrolledEvent(const MouseScrolledEvent& e);
		bool onMouseMovedEvent(const MouseMovedEvent& e);

		void setupImGuiStyle();
		void createImGuiDescriptorPool();
		void createImGuiFramebuffers();

		void createImGuiRenderPass(PipelineStages srcStageMask, AccessFlags srcAccess, VkRenderPass* renderPass); 
		void createImGuiRenderPass(); // imgui render pass is currently being created via this func
	private:
		bool m_blockEvents = false;

		VkRenderPass m_imGuiRenderPass;
		std::vector<VkFramebuffer> m_imGuiFramebuffers;
		VkDescriptorPool m_imGuiDescriptorPool;

		Ref<GraphicsPipeline> m_imGuiPipeline;
		Ref<Shader> m_imGuiShader;
		Scope<CommandBuffer> m_imGuiCmdBuffer;
		Scope<Semaphore> m_imGuiRenderCompleteSemaphore;

		struct RenderBuffers
		{
			Ref<VertexBuffer> vertexBuffer;
			Ref<IndexBuffer> indexBuffer;
		} m_imGuiBuffers;
	};
}