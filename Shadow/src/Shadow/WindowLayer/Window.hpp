#pragma once

struct GLFWwindow;

namespace Shadow
{
	enum class CursorMode
	{
		Normal = 212993,
		Hidden =  212995
	};

	class Window
	{
	public:
		Window(uint32_t width, uint32_t height, const char* title = "Shadow");
		~Window();

		inline void* getWindowHandle() const { return m_window; }
		void getFramebufferSize(uint32_t& outWidth, uint32_t& outHeight) const;
		
		inline int getWidth() const { return m_windowProperties.width; }
		inline int getHeight() const { return m_windowProperties.height; }
		inline float getAspectRatio() const { return m_windowProperties.aspectRatio; }

		void present();
		void setCursorMode(CursorMode cursorMode) const;

		static Scope<Window> create(uint32_t width, uint32_t height, const char* title = "Shadow");
	private:
		void setCallbacks();
	private:
		struct WindowProperties
		{
			uint32_t width, height;
			float aspectRatio;
			const char* title;
		} m_windowProperties;

		GLFWwindow* m_window;
	};
}


