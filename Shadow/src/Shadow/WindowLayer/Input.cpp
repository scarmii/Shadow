#include"shpch.hpp"
#include"Shadow/WindowLayer/Input.hpp"
#include"Shadow/Core/ShEngine.hpp"

#include<GLFW/glfw3.h>

namespace Shadow
{
	std::unique_ptr<Input> Input::s_instance = std::unique_ptr<Input>(new Input());

	bool Input::isKeyPressedImpl(int keycode) const
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(ShEngine::get().getWindow().getWindowHandle());
		int state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::isMouseButtonPressedImpl(int button) const
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(ShEngine::get().getWindow().getWindowHandle());
		int state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}
}