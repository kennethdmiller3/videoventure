#include "StdAfx.h"

namespace Platform
{
	bool OpenWindow(void)
	{
		glfwOpenWindowHint(GLFW_ACCUM_RED_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_GREEN_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_BLUE_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_ALPHA_BITS, 16);
		glfwOpenWindowHint(GLFW_FSAA_SAMPLES, OPENGL_MULTISAMPLE);
		if (!glfwOpenWindow(SCREEN_WIDTH, SCREEN_HEIGHT, 8, 8, 8, 8, 0, 0, SCREEN_FULLSCREEN ? GLFW_FULLSCREEN : GLFW_WINDOW))
			return false;
		glfwSetWindowTitle("Shmup!");
		glfwSwapInterval( OPENGL_SWAPCONTROL );
		return true;
	}

	void CloseWindow(void)
	{
		glfwCloseWindow();
	}
}
