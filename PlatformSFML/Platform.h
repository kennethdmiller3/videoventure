#pragma once

#if defined(WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#endif

// SFML includes
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <GL/gl.h>
#include <GL/glu.h>

#define USE_SFML

struct TextureTemplate;

namespace Platform
{
	extern sf::RenderWindow window;

	extern bool Init(void);
	extern bool OpenWindow(void); 
	extern void CloseWindow(void);
	extern void Done(void);

	// load texture data
	extern bool LoadTexture(TextureTemplate &aTexture, const char *aName);

	// show/hide the cursor
	inline void ShowCursor(bool aShow)
	{
		window.ShowMouseCursor(aShow);
	}

	// grab input
	inline void GrabInput(bool aGrab)
	{
	}

	// show back buffer
	inline void Present(void)
	{
		window.Display();
	}
}
