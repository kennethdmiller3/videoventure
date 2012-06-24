#include "StdAfx.h"

namespace Platform
{
	bool Init(void)
	{
		// initialize GLFW
		glfwInit();

		// open the window
		if (!OpenWindow())
			return false;

		// success!
		return true;
	}

	void Done(void)
	{
		// terminate GLFW
		glfwTerminate();
	}
}
