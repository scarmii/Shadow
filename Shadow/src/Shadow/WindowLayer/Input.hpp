#pragma once

namespace Shadow
{
	class Input
	{
	public:
		inline static bool isKeyPressed(int keycode) { return s_instance->isKeyPressedImpl(keycode); }
		inline static bool isMouseButtonPressed(int button) { return s_instance->isMouseButtonPressedImpl(button); }
	private:
		bool isKeyPressedImpl(int keycode) const;
		bool isMouseButtonPressedImpl(int button) const;
	private:
		static std::unique_ptr<Input> s_instance;
	};
}