#pragma once

#include "Shadow/Events/EventDispatcher.hpp"

namespace Shadow
{
	class Renderpass;

	class ImGuiLayer 
	{
	public:
		virtual ~ImGuiLayer() = default;

		virtual void begin() = 0;
		virtual void submit() = 0;
		virtual void updateWindows() = 0;

		static Ref<ImGuiLayer> create();
	protected:
		void setupImGuiStyle() const;
	};
}