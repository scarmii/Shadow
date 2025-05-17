#include "shpch.hpp"
#include "ShEngine.hpp"
#include "Shadow/Core/Core.hpp"
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Renderer/Renderer2D.hpp"
		 
#include <imgui.h>
#include <GLFW/glfw3.h>

// TODO: multithreading

namespace Shadow
{
	static std::chrono::steady_clock::time_point s_start;
	static std::future<void> s_asyncEventDispatcher;

	ShEngine::ShEngine()
		: m_frameRate(0.0f)
	{
		SH_ASSERT(!s_instance, "app already exists o^o");
		s_instance = this;

		EventDispatcher::init();
		EventDispatcher& dispatcher = EventDispatcher::get();
		dispatcher.addReciever(SH_CALLBACK(ShEngine::onWindowCloseEvent));
		dispatcher.addReciever(SH_CALLBACK(ShEngine::onWinResizedEvent));

		m_window = Window::create(1280, 720);
		Renderer::init();
		Renderer2D::init();
		m_imGuiLayer = ImGuiLayer::create();
	}

	ShEngine::~ShEngine()
	{
		EventDispatcher::shutdown();
		Renderer2D::shutdown();
		Renderer::shutdown();
	}

	void ShEngine::run()
	{
		while (m_running)
		{
			SH_PROFILE_SCOPE("RunLoop");

			float time = static_cast<float>(glfwGetTime()); // Platform::getTime()
			Timestep timestep = time - m_lastFrameTime;
			m_lastFrameTime = time;
			m_frameRate = timestep.getMilliseconds();

			// events are dispatched asynchronously (except WindowResized events, since they interact with rendering resources)
			s_asyncEventDispatcher = std::async(std::launch::async, &ShEngine::dispatchEventsAsync, this);
		    EventDispatcher::get().dispatch<WindowResizedEvent>();

			if (!m_minimized)
			{
				{
					SH_PROFILE_SCOPE("layerStack - onUpdate");

					for (Layer* layer : m_layerStack)
						layer->onUpdate(timestep);
				}

				const Ref<RenderCmdBuffer>& renderCmdBuffer = Renderer::getCmdBuffer();
				renderCmdBuffer->begin();
				{
					for (Layer* layer : m_layerStack)
						layer->onRender();
				}
				renderCmdBuffer->end(); 

				m_imGuiLayer->begin();
				{
					SH_PROFILE_SCOPE("layerStack - onImGuiRender");

					for (Layer* layer : m_layerStack)
						layer->onImGuiRender();
				}
				m_imGuiLayer->submit();
				m_imGuiLayer->updateWindows();

				renderCmdBuffer->submit();
			}

			s_asyncEventDispatcher.wait();
			m_window->present();
		}
	}

	void ShEngine::pushLayer(Layer* layer)
	{
		m_layerStack.pushLayer(layer);
	}

	void ShEngine::pushOverlay(Layer* layer)
	{
		m_layerStack.pushOverlay(layer);
	}

	bool ShEngine::onWindowCloseEvent(const WindowClosedEvent& event)
	{
		m_running = false;
		return true;
	}

	bool ShEngine::onWinResizedEvent(const WindowResizedEvent& event)
	{
		m_minimized = event.width == 0 && event.height == 0 ? true : false;
		return false;
	}

	void ShEngine::dispatchEventsAsync()
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

	void ShEngine::recordImguiCmdsAsync()
	{
		{
			SH_PROFILE_SCOPE("layerStack - onImGuiRender");
		
			for (Layer* layer : m_layerStack)
				layer->onImGuiRender();
		}
		m_imGuiLayer->submit();
	}
}