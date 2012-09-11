#include "StdAfx.h"

// input callbacks
extern void KeyCallback(GLFWwindow aWindow, int aKey, int aAction);
extern void CharCallback(GLFWwindow aWindow, int aChar);
extern void MousePosCallback(GLFWwindow aWindow, int aPosX, int aPosY);
extern void MouseButtonCallback(GLFWwindow aWindow, int aButton, int aAction);
extern void ScrollCallback(GLFWwindow aWindow, double aScrollX, double aScrollY);
extern int WindowCloseCallback(GLFWwindow aWindow);

namespace Platform
{
	static GLFWwindow sWindow;

	bool OpenWindow(void)
	{
		// set window hints
#ifdef ENABLE_ACCUMULATION_BUFFER
		glfwWindowHint(GLFW_ACCUM_RED_BITS, 16);
		glfwWindowHint(GLFW_ACCUM_GREEN_BITS, 16);
		glfwWindowHint(GLFW_ACCUM_BLUE_BITS, 16);
		glfwWindowHint(GLFW_ACCUM_ALPHA_BITS, 16);
#endif
		glfwWindowHint(GLFW_FSAA_SAMPLES, OPENGL_MULTISAMPLE);
		glfwWindowHint(GLFW_RED_BITS, 8);
		glfwWindowHint(GLFW_GREEN_BITS, 8);
		glfwWindowHint(GLFW_BLUE_BITS, 8);
#ifdef ENABLE_SRC_ALPHA_SATURATE
		glfwWindowHint(GLFW_ALPHA_BITS, 8);
#else
		glfwWindowHint(GLFW_ALPHA_BITS, 0);
#endif
#ifdef ENABLE_DEPTH
		glfwWindowHint(GLFW_DEPTH_BITS, 16);
#else
		glfwWindowHint(GLFW_DEPTH_BITS, 0);
#endif
		glfwWindowHint(GLFW_STENCIL_BITS, 0);
		
		// create the window
		sWindow = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
			SCREEN_FULLSCREEN ? GLFW_FULLSCREEN : GLFW_WINDOWED, "VideoVenture", NULL);
		if (!sWindow)
			return false;

		// set vertical sync
		glfwSwapInterval(OPENGL_SWAPCONTROL);

		// hide the mouse cursor
		ShowCursor(false);

		// set callbacks
		glfwSetKeyCallback(KeyCallback);
		glfwSetCharCallback(CharCallback);
		glfwSetCursorPosCallback(MousePosCallback);
		glfwSetMouseButtonCallback(MouseButtonCallback);
		glfwSetScrollCallback(ScrollCallback);
		glfwSetWindowCloseCallback(WindowCloseCallback);

		// make it the current context
		glfwMakeContextCurrent(sWindow);

		return true;
	}

	void CloseWindow(void)
	{
		glfwDestroyWindow(sWindow);
	}

	// show/hide the cursor
	void ShowCursor(bool aShow)
	{
		if (aShow)
			glfwSetInputMode(sWindow, GLFW_CURSOR_MODE, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(sWindow, GLFW_CURSOR_MODE, GLFW_CURSOR_CAPTURED);
	}

	// grab input
	void GrabInput(bool aGrab)
	{
	}

	// show back buffer
	void Present(void)
	{
		glfwSwapBuffers(sWindow);
	}
}
