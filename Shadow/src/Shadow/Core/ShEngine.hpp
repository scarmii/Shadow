#pragma once

#include "Shadow/Core/Core.hpp"
#include "Shadow/Events/EventDispatcher.hpp"
#include "Shadow/WindowLayer/Window.hpp"
#include "Shadow/Renderer/GraphicsContext.hpp"
#include "Shadow/Core/LayerStack.hpp"
#include "Shadow/ImGui/ImGuiLayer.hpp"
#include "Shadow/Core/Timestep.hpp"

#include <future>

namespace Shadow
{
	class Renderpass;

	class ShEngine
	{
	public:
		ShEngine();
		virtual ~ShEngine();
		ShEngine(const ShEngine& other) = delete;
		ShEngine(ShEngine&& other) = delete;

		inline static const ShEngine& get() { return *s_instance; }

		void run();

		inline const Window& getWindow() const { return *m_window; }
		inline float getFrameRate() const { return m_frameRate; }
		inline const Ref<ImGuiLayer>& getImGuiLayer() const { return m_imGuiLayer; }

		void pushLayer(Layer* layer);
		void pushOverlay(Layer* layer);
	private:
		bool onWindowCloseEvent(const WindowClosedEvent& event);
		bool onWinResizedEvent(const WindowResizedEvent& event);

		// all event types except WindowResizedEvent are dispatched asynchronously
		void dispatchEventsAsync();
		void recordImguiCmdsAsync();
	private:
		inline static ShEngine* s_instance{ nullptr };

		bool m_running = true, m_minimized = false;
		float m_frameRate;
		Scope<Window> m_window;
		Ref<ImGuiLayer> m_imGuiLayer;
		LayerStack m_layerStack;
		float m_lastFrameTime = 0.0f;

		std::array<std::future<void>, 5> m_asyncEventDispatchers{};
		std::future<void> m_asyncImguiCmdRecord;
	};

	// To be defined in client
	extern ShEngine* createApp();
}