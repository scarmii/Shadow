#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"
#include "Shadow/Renderer/Renderer.hpp"

#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"

#include <GLFW/glfw3.h>

namespace Shadow
{
	static const std::vector<const char*> s_layers = { "VK_LAYER_KHRONOS_validation"};

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				VK_TRACE(pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				VK_WARN(pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				VK_ERROR(pCallbackData->pMessage);
				break;
		}
		return VK_FALSE;
	}

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_windowHandle(windowHandle)
	{
	}

	VulkanContext::~VulkanContext()
	{
		delete m_vkDevice;

		if (m_validationEnabled)
			destroyDebugMessenger();

		vkDestroyInstance(m_vkInstance, nullptr);

		SH_TRACE("destroyed Vulkan context");
	}

	void VulkanContext::init()
	{
		SH_PROFILE_FUNCTION();

		s_vkCtx = &(static_cast<VulkanContext&>(GraphicsContext::getCtx()));

#ifdef SH_DEBUG 
		m_validationEnabled = true;
#elif SH_VULKAN_VALIDATION_LAYERS
		m_validationEnabled = true;
#else
		m_validationEnabled = false;
#endif
		createInstance();

		m_vkDevice = new VulkanDevice(m_vkInstance, m_windowHandle, s_layers);
		m_vkDevice->init(m_windowHandle);

		SH_TRACE("initialized Vulkan Context >*<");
	}

	void VulkanContext::presentImage()
	{
		as<VulkanCmdBuffer>(Renderer::getCmdBuffer())->queuePresent();
	}

	void VulkanContext::createInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Shadow";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();

#ifdef SH_PLATFORM_APPLE
		extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (m_validationEnabled)
			SH_ASSERT(validationLayerSupported(),
				"validation layers requested, but not available :(");

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_validationEnabled)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(s_layers.size());
			createInfo.ppEnabledLayerNames = s_layers.data();
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
			createInfo.enabledLayerCount = 0;

		VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_vkInstance));
		SH_TRACE("created Vulkan Instance! <3");

		if (m_validationEnabled)
			VK_CHECK_RESULT(createDebugMessenger(debugCreateInfo));

		PFN_vkCmdBeginRenderingKHR pVkCmdBeginRenderingKHR = VK_LOAD_FUNC(m_vkInstance, vkCmdBeginRenderingKHR);
		SH_ASSERT(pVkCmdBeginRenderingKHR, "unable to load vkCmdBeginRenderingKHR (probably \"VK_KHR_dynamic_rendering\" is not present)");

		PFN_vkCmdEndRenderingKHR pVkCmdEndRenderingKHR = VK_LOAD_FUNC(m_vkInstance, vkCmdEndRenderingKHR);
		SH_ASSERT(pVkCmdEndRenderingKHR, "unable to load vkCmdEndRenderingKHR (probably \"VK_KHR_dynamic_rendering\" is not present)")
	}

	void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	VkResult VulkanContext::createDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		auto func = VK_LOAD_FUNC(m_vkInstance, vkCreateDebugUtilsMessengerEXT);
		if (func)
			return func(m_vkInstance, &createInfo, nullptr, &m_debugMessenger);

		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void VulkanContext::destroyDebugMessenger()
	{
		auto func = VK_LOAD_FUNC(m_vkInstance, vkDestroyDebugUtilsMessengerEXT);
		if (func)
			func(m_vkInstance, m_debugMessenger, nullptr);
	}

	std::vector<const char*> VulkanContext::getRequiredExtensions() const
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		bool extsSupported = extensionsSupported(extensions);
		SH_ASSERT(extsSupported, "extension is not present ;<");

		if (m_validationEnabled)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	bool VulkanContext::extensionsSupported(const std::vector<const char*>& requiredExtensions) const
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		for (const char* requiredExt : requiredExtensions)
		{
			if (std::find_if(extensions.cbegin(), extensions.cend(),
				[requiredExt](const VkExtensionProperties& extension)
				{
					return !strcmp(requiredExt, extension.extensionName);
				}) == extensions.cend())
			{
				return false;
			}
		}
		return true;
	}

	bool VulkanContext::validationLayerSupported() const
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : s_layers)
		{
			bool layerFound = false;

			for (const auto& layer : availableLayers)
			{
				if (!strcmp(layerName, layer.layerName))
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound)
				return false;
		}
		return true;
	}
}