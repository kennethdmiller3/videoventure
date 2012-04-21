#include "StdAfx.h"

namespace Platform
{
	bool OpenWindow()
	{
		// set OpenGL attributes
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
#ifdef ENABLE_SRC_ALPHA_SATURATE
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
#else
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
#endif
		SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 16 );
		SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 16 );
		SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 16 );
#ifdef ENABLE_SRC_ALPHA_SATURATE
		SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 16 );
#else
		SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0 );
#endif
#ifndef ENABLE_DEPTH_TEST
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
#endif
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, OPENGL_MULTISAMPLE > 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, OPENGL_MULTISAMPLE );
		SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, OPENGL_SWAPCONTROL );

		// create the window
		unsigned int flags = SDL_OPENGL;
		if (SCREEN_FULLSCREEN)
			flags |= SDL_FULLSCREEN;
		if( SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, flags ) == NULL )
			return false;

		return true;
	}

	void CloseWindow()
	{
	}
}
