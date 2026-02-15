#include "shpch.h"
#include "shadow/core/shApp.h"
#include "shadow/core/core.h"
#include "shadow/renderer/renderer.h"
#include "shadow/renderer/renderer2D.h"
		 
#include <imgui.h>
#include <GLFW/glfw3.h>

// TODO: multithreading

#define USE_IMGUI_CMD_BUFFER

namespace Shadow
{
	static std::chrono::steady_clock::time_point s_start;
	static std::future<void> s_asyncEventDispatcher;

	ShApp::ShApp(const char* name)
		: m_frameRate(0.0f)
	{
		SH_ASSERT(!s_instance, "app already exists o^o");
		s_instance = this;

		EventDispatcher::init();
		EventDispatcher& dispatcher = EventDispatcher::get();
		dispatcher.addReciever(SH_ON_EVENT_FN(onWindowCloseEvent, WindowClosedEvent));
		dispatcher.addReciever(SH_ON_EVENT_FN(onWinResizedEvent, WindowResizedEvent));

		m_window = Window::create(1280, 720, name);
		Renderer::init();
		m_imGuiLayer = createScope<ImGuiLayer>();
		Renderer2D::init();
	}

	ShApp::~ShApp()
	{
		EventDispatcher::shutdown();
		Renderer2D::shutdown();
		Renderer::shutdown();
	}

	void ShApp::close()
	{
		m_running = false;
	}

	void ShApp::run()
	{
		while (m_running)
		{
			SH_PROFILE_SCOPE("RunLoop");

			float time = static_cast<float>(glfwGetTime()); 
			Timestep timestep = time - m_lastFrameTime;
			m_lastFrameTime = time;
			m_frameRate = timestep.getMilliseconds();

			 //events are dispatched asynchronously (except WindowResized events, since they interact with renderering resources)
			s_asyncEventDispatcher = std::async(std::launch::async, &ShApp::dispatchEventsAsync, this);
			{
				SH_PROFILE_SCOPE("dispatch window resized event - ShApp::run")
				EventDispatcher::get().dispatch<WindowResizedEvent>();
			}

			if (!m_minimized)
			{
				{
					SH_PROFILE_SCOPE("onUpdate");
					s_instance->onUpdate(timestep);
				}

				auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
				cmdBuffer->begin();
				s_instance->onRender();

#ifdef USE_IMGUI_CMD_BUFFER
				cmdBuffer->end();
				cmdBuffer->submit();
#endif
				m_imGuiLayer->begin();
				{
					SH_PROFILE_SCOPE("onImGuiRender");
					s_instance->onImGuiRender();
				}
				m_imGuiLayer->end();
				m_imGuiLayer->updateWindows();

#ifndef USE_IMGUI_CMD_BUFFER
				cmdBuffer->end();
				cmdBuffer->submit();
#endif
			}

			s_asyncEventDispatcher.wait();
			m_window->present();
		}
	}

	bool ShApp::onWindowCloseEvent(const WindowClosedEvent& event)
	{
		m_running = false;
		return true;
	}

	bool ShApp::onWinResizedEvent(const WindowResizedEvent& event)
	{
		m_minimized = event.width == 0 && event.height == 0 ? true : false;
		return false;
	}

	void ShApp::dispatchEventsAsync()
	{
		SH_PROFILE_FUNCTION();
		EventDispatcher* pDispatcher = &EventDispatcher::get();

		m_asyncEventDispatchers[0] = std::async(std::launch::async, &EventDispatcher::dispatch<WindowClosedEvent>, pDispatcher);
		m_asyncEventDispatchers[1] = std::async(std::launch::async, &EventDispatcher::dispatch<KeyEvent>, pDispatcher);
		m_asyncEventDispatchers[2] = std::async(std::launch::async, &EventDispatcher::dispatch<MouseButtonEvent>, pDispatcher);
		m_asyncEventDispatchers[3] = std::async(std::launch::async, &EventDispatcher::dispatch <MouseMovedEvent>, pDispatcher);
		m_asyncEventDispatchers[4] = std::async(std::launch::async, &EventDispatcher::dispatch<MouseScrolledEvent>, pDispatcher);

		for (auto& asyncDispatcher : m_asyncEventDispatchers)
			asyncDispatcher.wait();
	}

	void ShApp::recordImguiCmdsAsync()
	{
		{
			SH_PROFILE_SCOPE("layerStack - onImGuiRender");
			s_instance->onImGuiRender();
		}
		m_imGuiLayer->end();
	}
}