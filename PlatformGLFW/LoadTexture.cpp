#include "StdAfx.h"

// get texture template definition
#include "Texture.h"

namespace Platform
{
	bool LoadTexture(TextureTemplate &aTexture, const char *aName)
	{
		// read image
		GLFWimage image;
		if (!glfwReadImage(aName, &image, GLFW_ALPHA_MAP_BIT))
			return false;

		// components per pixel
		aTexture.mComponents = image.BytesPerPixel;

		// texture dimensions
		aTexture.mWidth = image.Width;
		aTexture.mHeight = image.Height;

		// texture format
		aTexture.mFormat = image.Format;

		// copy pixel data
		size_t count = aTexture.mWidth * aTexture.mHeight * aTexture.mComponents;
		aTexture.mPixels = static_cast<unsigned char *>(malloc(count));
		memcpy(aTexture.mPixels, image.Data, count);

		// free image
		glfwFreeImage(&image);

		// success!
		return true;
	}
}