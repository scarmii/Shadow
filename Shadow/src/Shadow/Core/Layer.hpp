#pragma once

#include "Shadow/Core/Timestep.hpp"

namespace Shadow
{
	class Layer
	{
	public:
		virtual ~Layer() {}

		virtual void onAttach() {}
		virtual void onDetach() {}
		virtual void onUpdate(Timestep ts) {}
		virtual void onRender() {}
		virtual void onImGuiRender() {}
	};
}