#pragma once

// SDL includes
#include "SDL.h"
#include "SDL_opengl.h"

#define USE_SDL

struct TextureTemplate;

namespace Platform
{
	extern bool Init(void);
	extern bool OpenWindow(void);
	extern void CloseWindow(void);
	extern void Done(void);

	// utilities

	// load texture data
	extern bool LoadTexture(TextureTemplate &aTexture, const char *aName);

	// show/hide the cursor
	inline void ShowCursor(bool aShow)
	{
		SDL_ShowCursor(aShow ? SDL_ENABLE : SDL_DISABLE);
	}
	
	// grab/release input
	inline void GrabInput(bool aGrab)
	{
		SDL_WM_GrabInput(aGrab ? SDL_GRAB_ON : SDL_GRAB_OFF);
	}

	// present the back buffer
	inline void Present(void)
	{
		SDL_GL_SwapBuffers();
	}
}
