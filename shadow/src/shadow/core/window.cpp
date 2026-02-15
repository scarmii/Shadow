#include "shpch.h"
#include "shadow/core/core.h"
#include "shadow/core/shApp.h"
#include "shadow/core/window.h"
#include "shadow/events/eventDispatcher.h"
#include "shadow/vulkan/context.h"
		 
#include <GLFW/glfw3.h>

namespace Shadow
{
	Window::Window(uint32_t width, uint32_t height, const char* title)
		: m_windowProperties{ width, height, static_cast<float>(width) / static_cast<float>(height), title }
	{
		int status = glfwInit();
		SH_ASSERT(status, "Failed to initialize GLFW :<");

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		SH_ASSERT(m_window, "Failed to create GLFW window :(");

		glfwSetWindowUserPointer(m_window, (void*)&m_windowProperties);

		VulkanContext::init(m_window);

		setCallbacks();
		glfwSwapInterval(1); 
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		VulkanContext::destroy();
	}

	void Window::getFramebufferSize(uint32_t& outWidth, uint32_t& outHeight) const
	{
		glfwGetFramebufferSize(m_window, (int*)&outWidth, (int*)&outHeight);
	}

	void Window::present()
	{
		SH_PROFILE_FUNCTION();

		{
			SH_PROFILE_SCOPE("glfwPollEvents - Window::present");
			glfwPollEvents();
		}
		VulkanContext::getCtx().presentImage();
	}

	void Window::setCursorMode(CursorMode cursorMode) const
	{
		glfwSetInputMode(m_window, GLFW_CURSOR, static_cast<int>(cursorMode));
	}

	Scope<Window> Window::create(uint32_t width, uint32_t height, const char* title)
	{
		return createScope<Window>(width, height, title);
	}

	void Window::setCallbacks()
	{
		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
		{
			EventDispatcher::get().addEvent(WindowClosedEvent{});
		});

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
			{
				WindowProperties& data = *(WindowProperties*)glfwGetWindowUserPointer(window);
				data.width = width;
				data.height = height;
				data.aspectRatio = static_cast<float>(width) / static_cast<float>(height);

				EventDispatcher::get().addEvent(WindowResizedEvent{ width, height });
		});

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			EventDispatcher::get().addEvent(KeyEvent{ key, action == GLFW_RELEASE ? false : true });
		});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
		{
			EventDispatcher::get().addEvent(MouseButtonEvent{ button, action == GLFW_RELEASE ? false : true });
		});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos)
		{
			EventDispatcher::get().addEvent(MouseMovedEvent{(float)xpos, (float)ypos});
		});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			EventDispatcher::get().addEvent(MouseScrolledEvent{ (float)xoffset, (float)yoffset });
		});
	}
}