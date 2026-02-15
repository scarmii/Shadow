#pragma once

#include "shadow/core/inputCodes.h"

#include <glm/glm.hpp>

namespace Shadow
{
	class Input
	{
	public:
		static bool isKeyPressed(KeyCode keycode);
		static bool isMouseButtonPressed(MouseCode button);

		static glm::vec2 getMousePosition();
	};
}