#include "StdAfx.h"

// get texture template definition
#include "..\Source\Texture.h"

namespace Platform
{
	bool LoadTexture(TextureTemplate &aTexture, const char *aName)
	{
		// read image
		GLFWimage image;
		if (!glfwReadImage(aName, &image, GLFW_ALPHA_MAP_BIT))
			return false;

		// components per pixel
		texture.mComponents = image.BytesPerPixel;

		// texture dimensions
		texture.mWidth = image.Width;
		texture.mHeight = image.Height;

		// texture format
		texture.mFormat = image.Format;

		// copy pixel data
		size_t count = texture.mWidth * texture.mHeight * texture.mComponents;
		texture.mPixels = static_cast<unsigned char *>(malloc(count));
		memcpy(texture.mPixels, image.Data, count);

		// free image
		glfwFreeImage(&image);

		// success!
		return true;
	}
}