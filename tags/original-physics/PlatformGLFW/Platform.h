#pragma once

#if defined(WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#endif

// GLFW includes
#include <GL/glfw.h>

#define USE_GLFW

struct TextureTemplate;

namespace Platform
{
	extern bool Init(void);
	extern bool OpenWindow(void); 
	extern void CloseWindow(void);
	extern void Done(void);

	// load texture data
	extern bool LoadTexture(TextureTemplate &aTexture, const char *aName);

	// show/hide the cursor
	inline void ShowCursor(bool aShow)
	{
		if (aShow)
			glfwEnable(GLFW_MOUSE_CURSOR);
		else
			glfwDisable(GLFW_MOUSE_CURSOR);
	}

	// grab input
	inline void GrabInput(bool aGrab)
	{
	}

	// show back buffer
	inline void Present(void)
	{
		glfwSwapBuffers();
	}
}
