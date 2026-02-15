#pragma once

#include "shadow/core/timestep.h"

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