#pragma once

// texture descriptor
struct TextureTemplate
{
	GLint mInternalFormat;
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

extern void BindTexture(GLuint handle, TextureTemplate const &texture);
extern void RebuildTextures(void);

namespace Database
{
	extern Typed<TextureTemplate> texturetemplate;
	extern Typed<GLuint> texture;
}
