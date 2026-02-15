#pragma once

#include "shadow/core/core.h"
#include "shadow/vulkan/device.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct GLFWwindow;

namespace Shadow
{
	class VulkanContext
	{
	public:
	   ~VulkanContext();

		static void init(GLFWwindow* windowHandle);
		static void destroy();

		void presentImage();

		inline static VulkanContext& getCtx() { return *s_vkCtx; }
		inline static Device* const getDevice() { return s_vkCtx->m_vkDevice; }
		inline static const VkInstance const getVkInstance() { return s_vkCtx->m_vkInstance; }
	private:
		VulkanContext(GLFWwindow* windowHandle);

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
		
		Device* m_vkDevice;
	};
}