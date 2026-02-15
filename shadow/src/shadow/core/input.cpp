#include "shpch.h"
#include "shadow/core/input.h"
#include "shadow/core/shApp.h"

#include<GLFW/glfw3.h>

namespace Shadow
{
	bool Input::isKeyPressed(KeyCode keycode) 
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(ShApp::get().getWindow().getWindowHandle());
		int state = glfwGetKey(window, static_cast<int>(keycode));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::isMouseButtonPressed(MouseCode button) 
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(ShApp::get().getWindow().getWindowHandle());
		int state = glfwGetMouseButton(window, static_cast<int>(button));
		return state == GLFW_PRESS;
	}

	glm::vec2 Input::getMousePosition()
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(ShApp::get().getWindow().getWindowHandle());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return { static_cast<float>(xpos), static_cast<float>(ypos) };
	}
}