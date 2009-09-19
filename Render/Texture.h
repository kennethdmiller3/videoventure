#pragma once

// texture descriptor
struct TextureTemplate
{
	GLint mComponents;
	GLint mWidth;
	GLint mHeight;
	GLenum mFormat;
	unsigned char *mPixels;
	GLint mEnvMode;
	GLint mMinFilter;
	GLint mMagFilter;
	GLint mWrapS;
	GLint mWrapT;
};

extern void RebuildTextures(void);

namespace Database
{
	extern Typed<TextureTemplate> texturetemplate;
	extern Typed<GLuint> texture;
}
