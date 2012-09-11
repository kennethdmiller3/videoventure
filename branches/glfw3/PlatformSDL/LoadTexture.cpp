#include "StdAfx.h"

// get texture template definition
#include "Texture.h"

namespace Platform
{
	bool LoadTexture(TextureTemplate &aTexture, const char *aName)
	{
		// get the surface
		SDL_Surface *surface = SDL_LoadBMP(aName);
		if (!surface)
		{
			DebugPrint("error: could not open %s\n", aName);
			return false;
		}

		// check if the width is a power of 2
		if ((surface->w & (surface->w - 1)) != 0)
		{
			DebugPrint("warning: %s width %d is not a power of 2\n", aName, surface->w);
		}
		
		// check if the height is a power of 2
		if ((surface->h & (surface->h - 1)) != 0)
		{
			DebugPrint("warning: %s height %d is not a power of 2\n", aName, surface->h);
		}
	 
		// components per pixel
		aTexture.mFormat = surface->format->BytesPerPixel;

		// texture dimensions
		aTexture.mWidth = surface->w;
		aTexture.mHeight = surface->h;

		// get the number of channels in the SDL surface
		if (aTexture.mFormat == 4)     // contains an alpha channel
		{
			if (surface->format->Rmask == 0x000000ff)
				aTexture.mInternalFormat = GL_RGBA;
			else
				aTexture.mInternalFormat = GL_BGRA;
		}
		else if (aTexture.mFormat == 3)     // no alpha channel
		{
			if (surface->format->Rmask == 0x000000ff)
				aTexture.mInternalFormat = GL_RGB;
			else
				aTexture.mInternalFormat = GL_BGR;
		}
		else
		{
			DebugPrint("warning: %s is not truecolor\n", aName);
			aTexture.mInternalFormat = GL_LUMINANCE;
		}

		// copy pixel data
		size_t count = aTexture.mWidth * aTexture.mHeight * surface->format->BytesPerPixel;
		aTexture.mPixels = static_cast<unsigned char *>(malloc(count));
		memcpy(aTexture.mPixels, surface->pixels, count);

		// done with surface
		SDL_FreeSurface(surface);

		// success!
		return true;
	}
}