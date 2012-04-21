#include "StdAfx.h"

namespace Platform
{
	void PrintAttributes(void)
	{
		int value;

		// current resolution
		SDL_Surface *surface = SDL_GetVideoSurface();
		SDL_PixelFormat *format = surface->format;
		DebugPrint("\nCurrent: %dx%d %dbpp\n", surface->w, surface->h, format->BitsPerPixel);

		// get fullscreen resolutions
		DebugPrint("\nResolutions:\n");
		SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
		for (SDL_Rect **mode = modes; *mode != NULL; ++mode)
			DebugPrint("%dx%d\n", (*mode)->w, (*mode)->h);

		const char *attrib[] =
		{
			"SDL_GL_RED_SIZE",
			"SDL_GL_GREEN_SIZE",
			"SDL_GL_BLUE_SIZE",
			"SDL_GL_ALPHA_SIZE",
			"SDL_GL_BUFFER_SIZE",
			"SDL_GL_DOUBLEBUFFER",
			"SDL_GL_DEPTH_SIZE",
			"SDL_GL_STENCIL_SIZE",
			"SDL_GL_ACCUM_RED_SIZE",
			"SDL_GL_ACCUM_GREEN_SIZE",
			"SDL_GL_ACCUM_BLUE_SIZE",
			"SDL_GL_ACCUM_ALPHA_SIZE",
			"SDL_GL_STEREO",
			"SDL_GL_MULTISAMPLEBUFFERS",
			"SDL_GL_MULTISAMPLESAMPLES",
			"SDL_GL_ACCELERATED_VISUAL",
			"SDL_GL_SWAP_CONTROL"
		};
		for (int i = 0; i < SDL_arraysize(attrib); ++i)
		{
			SDL_GL_GetAttribute( SDL_GLattr(i), &value );
			DebugPrint( "%s: %d\n", attrib[i], value);
		}
	}
}