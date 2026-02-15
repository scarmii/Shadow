#pragma once

#include "shadow/core/core.h"
#include "shadow/core/timestep.h"
#include "shadow/core/layerStack.h"
#include "shadow/core/types.h"
#include "shadow/core/window.h"

#include "shadow/events/eventDispatcher.h"
#include "shadow/imGui/imguiLayer.h"

#include <future>

int main(int argc, char** argv);

namespace Shadow
{
	class RenderPass;

	class ShApp
	{
	public:
		ShApp(const char* name = "shadow app");
		virtual ~ShApp();
		ShApp(const ShApp& other) = delete;
		ShApp(ShApp&& other) = delete;

		inline static ShApp& get() { return *s_instance; }

		virtual void onUpdate(Timestep ts) {}
		virtual void onRender() {}
		virtual void onImGuiRender() {}

		void close();

		inline const Window& getWindow() const { return *m_window; }
		inline const Scope<ImGuiLayer>& getImGuiLayer() const { return m_imGuiLayer; }
		inline float getFrameRate() const { return m_frameRate; }
		inline bool windowMinimized() const { return m_minimized; }
	private:
		void run();
		bool onWindowCloseEvent(const WindowClosedEvent& event);
		bool onWinResizedEvent(const WindowResizedEvent& event);

		// all event types except WindowResizedEvent are dispatched asynchronously
		void dispatchEventsAsync();
		void recordImguiCmdsAsync();
	private:
		inline static ShApp* s_instance{ nullptr };
		friend int ::main(int argc, char** argv);

		bool m_running = true, m_minimized = false;
		float m_frameRate;
		Scope<Window> m_window;
		Scope<ImGuiLayer> m_imGuiLayer;
		float m_lastFrameTime = 0.0f;

		std::array<std::future<void>, 5> m_asyncEventDispatchers{};
		std::future<void> m_asyncImguiCmdRecord;
	};

	// To be defined in client
	extern ShApp* createApp();
}