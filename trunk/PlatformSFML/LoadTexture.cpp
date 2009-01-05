#include "StdAfx.h"

// get texture template definition
#include "..\Source\Texture.h"

namespace Platform
{
	bool LoadTexture(TextureTemplate &aTexture, const char *aName)
	{
		// read image
		sf::Image image;
		if (!image.LoadFromFile(aName))
			return false;

		// components per pixel
		texture.mComponents = 4;

		// texture dimensions
		texture.mWidth = image.GetWidth();
		texture.mHeight = image.GetHeight();

		// texture format
		texture.mFormat = GL_RGBA;

		// copy pixel data
		size_t count = texture.mWidth * texture.mHeight * texture.mComponents;
		texture.mPixels = static_cast<unsigned char *>(malloc(count));
		memcpy(texture.mPixels, image.GetPixelsPtr(), count);

		// success!
		return true;
	}
}