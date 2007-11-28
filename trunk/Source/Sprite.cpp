#include "StdAfx.h"
#include "Sprite.h"

Sprite::Sprite(void)
: texture(0)
, u0(0), v0(0), u1(1), v1(1)
, x0(0), y0(0), x1(0), y1(0)
{
}

Sprite::~Sprite(void)
{
	if (texture)
		glDeleteTextures( 1, &texture );
}

void Sprite::SetTexture(const char *aName)
{
	// load the specified bitmap
	SDL_Surface *surface = SDL_LoadBMP(aName);
	if ( surface )
	{ 	 
		// check if the width is a power of 2
		if ( (surface->w & (surface->w - 1)) != 0 )
		{
			DebugPrint("warning: %s width %d is not a power of 2\n", aName, surface->w);
		}
		
		// check if the height is a power of 2
		if ( (surface->h & (surface->h - 1)) != 0 )
		{
			DebugPrint("warning: %s height %d is not a power of 2\n", aName, surface->h);
		}
	 
		// get the number of channels in the SDL surface
		GLenum texture_format;
		GLint nOfColors = surface->format->BytesPerPixel;
		if (nOfColors == 4)     // contains an alpha channel
		{
			if (surface->format->Rmask == 0x000000ff)
				texture_format = GL_RGBA;
			else
				texture_format = GL_BGRA;
		}
		else if (nOfColors == 3)     // no alpha channel
		{
			if (surface->format->Rmask == 0x000000ff)
				texture_format = GL_RGB;
			else
				texture_format = GL_BGR;
		}
		else
		{
			DebugPrint("warning: %s is not truecolor\n", aName);
			SDL_FreeSurface( surface );
			return;
		}
	        
		// generate a texture object handle
		glGenTextures( 1, &texture );
	 
		// bind the texture object
		glBindTexture( GL_TEXTURE_2D, texture );
	 
		// set minification and magnification properties
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	 
		// set texture image data
		glTexImage2D(
			GL_TEXTURE_2D, 0, nOfColors, surface->w, surface->h, 0,
			texture_format, GL_UNSIGNED_BYTE, surface->pixels
			);

		// free the surface
		SDL_FreeSurface( surface );
	} 
	else
	{
		printf("SDL could not load %s: %s\n", aName, SDL_GetError());
	}    
}

void Sprite::Render(void)
{
	// if using a texture...
	if (texture)
	{
		// enable 2D texturing
		glEnable( GL_TEXTURE_2D );

		// bind texture
		glBindTexture( GL_TEXTURE_2D, texture );
	}
	else
	{
		// disable 2D texturing
		glDisable( GL_TEXTURE_2D );
	}

	// begin primitive
	glBegin( GL_QUADS );

	// submit vertices
	glTexCoord2f( u0, v0 );
	glVertex3f( x0, y0, 0 );
	glTexCoord2f( u1, v0 );
	glVertex3f( x1, y0, 0 );
	glTexCoord2f( u1, v1 );
	glVertex3f( x1, y1, 0 );
	glTexCoord2f( u0, v1 );
	glVertex3f( x0, y1, 0 );

	// end primitive
	glEnd();

#ifdef DRAW_COORDINATE_AXES
	glDisable( GL_TEXTURE_2D );
	glBegin( GL_LINES );
	glColor4f(1, 0, 0, 1);
	glVertex3f( 0, 0, 0 );
	glVertex3f( 16, 0, 0);
	glColor4f(0, 1, 0, 1);
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 16, 0);
	glEnd();
#endif
}
