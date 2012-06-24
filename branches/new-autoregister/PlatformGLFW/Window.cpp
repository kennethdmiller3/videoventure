#include "StdAfx.h"

// input callbacks
extern void KeyCallback(int aIndex, int aState);
extern void CharCallback(int aIndex, int aState);
extern void MousePosCallback(int aPosX, int aPosY);
extern void MouseButtonCallback(int aIndex, int aState);
extern void MouseWheelCallback(int aPos);

namespace Platform
{
	bool OpenWindow(void)
	{
		// set window hints
		glfwOpenWindowHint(GLFW_ACCUM_RED_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_GREEN_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_BLUE_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_ALPHA_BITS, 16);
		glfwOpenWindowHint(GLFW_FSAA_SAMPLES, OPENGL_MULTISAMPLE);

		// create the window
		if (!glfwOpenWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
#ifdef ENABLE_SRC_ALPHA_SATURATE
			8, 8, 8, 8,
#else
			8, 8, 8, 0,
#endif
#ifdef ENABLE_DEPTH
			16, 0,
#else
			0, 0,
#endif
			SCREEN_FULLSCREEN ? GLFW_FULLSCREEN : GLFW_WINDOW))
			return false;

		// set the title
		glfwSetWindowTitle("Shmup!");

		// set vertical sync
		glfwSwapInterval( OPENGL_SWAPCONTROL );

		// hide the mouse cursor
		glfwDisable(GLFW_MOUSE_CURSOR);

		// set callbacks
		glfwSetKeyCallback(KeyCallback);
		glfwSetCharCallback(CharCallback);
		glfwSetMousePosCallback(MousePosCallback);
		glfwSetMouseButtonCallback(MouseButtonCallback);
		glfwSetMouseWheelCallback(MouseWheelCallback);

		return true;
	}

	void CloseWindow(void)
	{
		glfwCloseWindow();
	}
}
