#include "shpch.h"
#include "shadow/core/core.h"

#include "shadow/vulkan/swapchain.h"
#include "shadow/vulkan/context.h"
		 
#include <GLFW/glfw3.h>
#include <minmax.h>

namespace Shadow
{
	Swapchain::Swapchain(GLFWwindow* windowHandle, VkSurfaceKHR surface)
		: m_windowHandle(windowHandle), m_surface(surface)
	{
		createSwapchain();
		createImageViews();

		SH_TRACE("created a swapchain: {VkSwapchainKHR = %x}", m_swapchain);

		for (uint32_t i = 0; i < m_imageCount; i++)
			SH_TRACE("swapchain image[%u]: {VkImage = %x; VkImageView = %x}", i, m_images[i], m_imageViews[i]);

		EventDispatcher::get().addReciever(SH_ON_EVENT_FN(onWindowResized, WindowResizedEvent));
	}

	Swapchain::~Swapchain()
	{
		vkDestroySwapchainKHR(VulkanContext::getDevice()->getVkDevice(), m_swapchain, nullptr);

		for (size_t i = 0; i < m_imageViews.size(); i++)
			vkDestroyImageView(VulkanContext::getDevice()->getVkDevice(), m_imageViews[i], nullptr);
	}

	void Swapchain::acquireNextImage(VkSemaphore toSignalSemaphore)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		if (vkAcquireNextImageKHR(VulkanContext::getDevice()->getVkDevice(), m_swapchain, UINT64_MAX, toSignalSemaphore, VK_NULL_HANDLE, &m_imageIndex)
			== VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreate();
			SH_TRACE("vkAcquireNextImageKHR(...) -> recereated a swapchain");
		}
	}

	void Swapchain::presentImage(VkSemaphore renderCompleteSemaphore)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.pImageIndices = &m_imageIndex;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphore;

		if (vkQueuePresentKHR(VulkanContext::getDevice()->getGraphicsQueue(), &presentInfo) == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreate();
			SH_TRACE("vkQueuePresentKHR(...) -> recereated a swapchain");
		}
	}

	void Swapchain::createSwapchain()
	{
		SH_PROFILE_FUNCTION();

		Device* device = VulkanContext::getDevice();
		SurfaceDetails surfaceSupport = querySurfaceSupport(device->getPhysicalDevice(), m_surface);

		m_surfaceFormat = chooseSurfaceFormat(surfaceSupport.formats);
		VkPresentModeKHR presentMode = choosePresentMode(surfaceSupport.presentModes);
		m_extent = chooseExtent(surfaceSupport.capabilities);
		m_imageCount = min(surfaceSupport.capabilities.maxImageCount, surfaceSupport.capabilities.minImageCount + 1);

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface;
		createInfo.minImageCount = m_imageCount;
		createInfo.imageFormat = m_surfaceFormat.format;
		createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
		createInfo.imageExtent = m_extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueIndices[2] = { device->getGraphicsQueueIndex(), device->getPresentQueueIndex() };
		if (queueIndices[0] != queueIndices[1])
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = { queueIndices };
		}
		else
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		createInfo.preTransform = surfaceSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateSwapchainKHR(device->getVkDevice(), &createInfo, nullptr, &m_swapchain));
		vkGetSwapchainImagesKHR(device->getVkDevice(), m_swapchain, &m_imageCount, nullptr);
		vkGetSwapchainImagesKHR(device->getVkDevice(), m_swapchain, &m_imageCount, m_images.data());
	}

	void Swapchain::createImageViews()
	{
		SH_PROFILE_FUNCTION();

		for (size_t i = 0; i < m_imageCount; i++)
			m_imageViews[i] = VulkanContext::getDevice()->createImageView(m_images[i], m_imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

	void Swapchain::recreate()
	{
		SH_PROFILE_FUNCTION();

		Device* device = VulkanContext::getDevice();
		vkDestroySwapchainKHR(device->getVkDevice(), m_swapchain, nullptr);

		for (size_t i = 0; i < m_imageViews.size(); i++)
			vkDestroyImageView(device->getVkDevice(), m_imageViews[i], nullptr);

		createSwapchain();
		createImageViews();
	}

	Swapchain::SurfaceDetails Swapchain::querySurfaceSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SurfaceDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		SH_ASSERT(formatCount != 0, "Failed to create vulkan swapchain: formats haven't been found :<");
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		SH_ASSERT(presentModeCount != 0, "Failed to create vulkan swapchain: present modes haven't been found :<");
		details.presentModes.resize(presentModeCount);				 
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());

		return details;
	}

	VkSurfaceFormatKHR Swapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				m_imageFormat = format.format;
				return format;
			}
		}
		return availableFormats[0];
	}

	VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes)
	{
		for (const auto& presentMode : presentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return presentMode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_windowHandle, &width, &height);

		VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		extent.width = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return extent;
	}

	bool Swapchain::onWindowResized(const WindowResizedEvent& event)
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_windowHandle, &width, &height);

		// in case of window minimization
		while (width == 0 || height == 0)
		{
			SH_TRACE("win minimazed");
			glfwGetFramebufferSize(m_windowHandle, &width, &height);
			glfwWaitEvents();
		}

		Device* device = VulkanContext::getDevice();

		vkDestroySwapchainKHR(device->getVkDevice(), m_swapchain, nullptr);

		for (size_t i = 0; i < m_imageViews.size(); i++)
			vkDestroyImageView(device->getVkDevice(), m_imageViews[i], nullptr);

		createSwapchain();
		createImageViews();

		return false;
	}
}