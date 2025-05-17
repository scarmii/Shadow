#pragma once

#include "Shadow/Core/Core.hpp"
#include "Shadow/Renderer/GraphicsContext.hpp"

#include "Shadow/Vulkan/VulkanDevice.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct GLFWwindow;

namespace Shadow
{
	class VulkanCmdBuffer; 

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual ~VulkanContext();

		virtual void init() override;
		virtual void presentImage() override;

		inline static VulkanDevice* const getVulkanDevice() { return s_vkCtx->m_vkDevice; }
		inline static const VkInstance const getVkInstance() { return s_vkCtx->m_vkInstance; }
	private:
		void createInstance();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		VkResult createDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void destroyDebugMessenger();
		std::vector<const char*> getRequiredExtensions() const;
		bool extensionsSupported(const std::vector<const char*>& requiredExtensions) const;
		bool validationLayerSupported() const;
	private:
		inline static VulkanContext* s_vkCtx{ nullptr };

		GLFWwindow* m_windowHandle;
		VkInstance m_vkInstance;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		bool m_validationEnabled;
		
		VulkanDevice* m_vkDevice;
	};
}