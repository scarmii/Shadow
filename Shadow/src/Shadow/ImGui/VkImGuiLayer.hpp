#pragma once

#include "Shadow/ImGui/ImGuiLayer.hpp"
#include "Shadow/Vulkan/VulkanRenderpass.hpp"
#include "Shadow/Vulkan/VulkanPipeline.hpp"

#include <imgui/backends/imgui_impl_vulkan.h>

namespace Shadow
{
	class VulkanImGuiLayer : public ImGuiLayer
	{
	public:
		VulkanImGuiLayer();
		virtual ~VulkanImGuiLayer();

		virtual void begin() override;
		virtual void submit() override;
		virtual void updateWindows() override;

		inline VkSemaphore getRenderCompleteSemaphore(uint32_t currentFrame) const { return m_uiRenderCompleteSemaphores[currentFrame]; }
		inline VkCommandBuffer getCmdBuffer(uint32_t currentFrame) const { return m_imguiCmdBuffers[currentFrame]; }
	private:
		bool onWindowResized(const WindowResizedEvent& e);

		void createImGuiDescriptorPool();
		void createImGuiCmdBuffers();
		void createImGuiSyncObjects();
		void createImGuiRenderpass();
		void createImGuiFramebuffers();
	private:
		VkDescriptorPool m_imGuiDescriptorPool;

		VkCommandPool m_imguiCmdPool;
		std::array<VkCommandBuffer, VulkanDevice::s_maxFramesInFlight> m_imguiCmdBuffers;
		std::array<VkFence, VulkanDevice::s_maxFramesInFlight> m_inFlightFences;
		std::array<VkSemaphore, VulkanDevice::s_maxFramesInFlight> m_uiRenderCompleteSemaphores;

		VkRenderPass m_imguiRenderpass;
		std::vector<VkFramebuffer> m_imguiFramebuffers;
	};
}