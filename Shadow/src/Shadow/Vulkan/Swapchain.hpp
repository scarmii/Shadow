#pragma once

#include"Shadow/Events/EventDispatcher.hpp"

#include<vulkan/vulkan.h>
#include<future>
#include<vector>

struct GLFWwindow;

namespace Shadow
{
	class Swapchain
	{
	public:
		Swapchain(GLFWwindow* windowHandle, VkSurfaceKHR surface);
		~Swapchain();

		void acquireNextImage(VkSemaphore toSignalSemaphore);

		inline const VkExtent2D getExtent() const { return m_extent; }
		inline const uint32_t getImageCount() const { return m_imageCount; }
		inline const VkFormat getImageFormat() const { return m_imageFormat; }
		inline const std::array<VkImage, 3>& getImages() const { return m_images; }
		inline const std::array<VkImageView, 3>& getImageViews() const { return m_imageViews; }
		inline uint32_t getCurrentImageIndex() const { return m_imageIndex; }
		inline const VkSwapchainKHR getVkSwapchain() const { return m_swapchain; }
		inline const VkImageView getCurrentImageView() const { return m_imageViews[m_imageIndex]; }
		inline const VkImage getCurrentImage() const { return m_images[m_imageIndex]; }
	private:
		GLFWwindow* m_WindowHandle;

		uint32_t m_imageIndex = 0;
		uint32_t m_imageCount;

		VkSwapchainKHR m_swapchain;
		VkSurfaceKHR m_surface;
		VkSurfaceFormatKHR m_surfaceFormat;
		VkFormat m_imageFormat;
		VkExtent2D m_extent;

		std::array<VkImage, 3> m_images;
		std::array<VkImageView, 3> m_imageViews;

		struct SurfaceDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};
	private:
		void createSwapchain();
		void createImageViews();

		SurfaceDetails querySurfaceSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes);
		VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		bool recreateSwapchain(const WindowResizedEvent& event);
	};
}