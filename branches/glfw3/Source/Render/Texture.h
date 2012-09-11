#pragma once

// texture descriptor
struct TextureTemplate
{
	GLint mInternalFormat;
	GLint mWidth;
	GLint mHeight;
	GLenum mFormat;
	bool mMipmaps;
	bool mAllocated;
	unsigned char *mPixels;
	GLint mMinFilter;
	GLint mMagFilter;
	GLint mWrapS;
	GLint mWrapT;

	TextureTemplate()
		: mInternalFormat(0)
		, mWidth(0)
		, mHeight(0)
		, mFormat(0)
		, mMipmaps(false)
		, mAllocated(false)
		, mPixels(NULL)
		, mMinFilter(0)
		, mMagFilter(0)
		, mWrapS(0)
		, mWrapT(0)
	{
	}

	~TextureTemplate()
	{
		Free();
	}

	void Allocate(GLint aBytesPerPixel)
	{
		Free();
		mAllocated = true;
		mPixels = static_cast<unsigned char *>(malloc(mWidth * mHeight * aBytesPerPixel));
	}

	void Assign(unsigned char *aPixels)
	{
		Free();
		mAllocated = false;
		mPixels = aPixels;
	}

	void Free(void)
	{
		if (mAllocated)
			free(mPixels);
		mAllocated = false;
		mPixels = NULL;
	}
};

extern void BindTexture(GLuint handle, TextureTemplate const &texture);
extern void CleanupTextures(void);
extern void RebuildTextures(void);

namespace Database
{
	extern Typed<TextureTemplate> texturetemplate;
	extern Typed<GLuint> texture;
}
