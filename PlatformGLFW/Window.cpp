#include "StdAfx.h"

// input callbacks
extern void KeyCallback(int aIndex, int aState);
extern void CharCallback(int aIndex, int aState);
extern void MousePosCallback(int aPosX, int aPosY);
extern void MouseButtonCallback(int aIndex, int aState);
extern void MouseWheelCallback(int aPos);
extern int WindowCloseCallback();

namespace Platform
{
	bool OpenWindow(void)
	{
		// set window hints
#ifdef ENABLE_ACCUMULATION_BUFFER
		glfwOpenWindowHint(GLFW_ACCUM_RED_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_GREEN_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_BLUE_BITS, 16);
		glfwOpenWindowHint(GLFW_ACCUM_ALPHA_BITS, 16);
#endif
		glfwOpenWindowHint(GLFW_FSAA_SAMPLES, OPENGL_MULTISAMPLE);
		glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 2);
		//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
		//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
		//glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// not ready for this yet...
#ifdef DEBUG
		glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif

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
		glfwSetWindowCloseCallback(WindowCloseCallback);

		return true;
	}

	void CloseWindow(void)
	{
		glfwCloseWindow();
	}
}
