#include "StdAfx.h"

// input callbacks
extern void KeyCallback(int aIndex, int aState);
extern void MousePosCallback(int aPosX, int aPosY);
extern void MouseButtonCallback(int aIndex, int aState);
extern void MouseWheelCallback(int aPos);

namespace Platform
{
	bool Init(void)
	{
		// initialize GLFW
		glfwInit();

		// initialize the window
		if (!InitWindow())
			return false;

		// hide the mouse cursor
		glfwDisable(GLFW_MOUSE_CURSOR);

		// set callbacks
		glfwSetKeyCallback(KeyCallback);
		glfwSetMousePosCallback(MousePosCallback);
		glfwSetMouseButtonCallback(MouseButtonCallback);
		glfwSetMouseWheelCallback(MouseWheelCallback);

		// success!
		return true;
	}

	void Done(void)
	{
		// terminate GLFW
		glfwTerminate();
	}
}
