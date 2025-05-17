#pragma once

struct GLFWwindow;

namespace Shadow
{
	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void init() = 0;
		virtual void presentImage() = 0;

		inline static GraphicsContext& getCtx() { return *s_ctx; }

		static void destroy();
		static void create(GLFWwindow* windowHandle);
	private:
		inline static GraphicsContext* s_ctx{ nullptr };
	};
}