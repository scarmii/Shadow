#pragma once

#include "Shadow/Core/Log.hpp"

namespace Shadow
{
	struct WindowClosedEvent
	{
		WindowClosedEvent(WindowClosedEvent&& other) noexcept = default;

		WindowClosedEvent& operator=(WindowClosedEvent&& other) noexcept = default;
	};

	struct WindowResizedEvent
	{
		WindowResizedEvent(int newWidth, int newHeight)
			: width(newWidth), height(newHeight)
		{
		}

		WindowResizedEvent(WindowResizedEvent&& other) noexcept
		{
			width = std::move(other.width);
			height = std::move(other.height);
		}

		WindowResizedEvent& operator=(WindowResizedEvent&& other) noexcept
		{
			width = std::move(other.width);
			height = std::move(other.height);

			return *this;
		}

		int width, height;
	};

	struct KeyEvent
	{
		KeyEvent(int keycode, bool pressed)
			: keycode(keycode), pressed(pressed)
		{
		}

		KeyEvent(KeyEvent&& other) noexcept
		{
			keycode = std::move(other.keycode);
			pressed = std::move(other.pressed);
		}

		KeyEvent& operator=(KeyEvent&& other) noexcept
		{
			keycode = std::move(other.keycode);
			pressed = std::move(other.pressed);

			return *this;
		}

		int keycode;
		bool pressed;
	};

	struct MouseButtonEvent
	{
		MouseButtonEvent(int button, bool pressed)
			: button(button), pressed(pressed)
		{
		}

		MouseButtonEvent(MouseButtonEvent&& other) noexcept
		{
			button = std::move(other.button);
			pressed = std::move(other.pressed);
		}

		MouseButtonEvent& operator=(MouseButtonEvent&& other) noexcept
		{
			button = std::move(other.button);
			pressed = std::move(other.pressed);

			return *this;
		}

		int button;
		bool pressed;
	};

	struct MouseMovedEvent
	{
		MouseMovedEvent(float x, float y)
			: x(x), y(y)
		{
		}

		MouseMovedEvent(MouseMovedEvent&& other) noexcept
		{
			x = std::move(other.x);
			y = std::move(other.y);
		}

		MouseMovedEvent& operator=(MouseMovedEvent&& other) noexcept
		{
			x = std::move(other.x);
			y = std::move(other.y);

			return *this;
		}

		float x, y;
	};

	struct MouseScrolledEvent
	{
		MouseScrolledEvent(float xoffset, float yoffset)
			: xOffset(xoffset), yOffset(yoffset)
		{
		}

		MouseScrolledEvent(MouseScrolledEvent&& other) noexcept
		{
			xOffset = std::move(other.xOffset);
			yOffset = std::move(other.yOffset);
		}

		MouseScrolledEvent& operator=(MouseScrolledEvent&& other) noexcept
		{
			xOffset = std::move(other.xOffset);
			yOffset = std::move(other.yOffset);

			return *this;
		}

		float xOffset, yOffset;
	};
}