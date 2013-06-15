#pragma once

#if defined(WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#endif

// GLFW includes
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

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
	extern void ShowCursor(bool aShow);

	// grab input
	extern void GrabInput(bool aGrab);

	// show back buffer
	extern void Present(void);
}
