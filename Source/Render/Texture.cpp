#include "StdAfx.h"

#include "Texture.h"
#include "ExpressionConfigure.h"
#include "Noise.h"

static const char * sColorNames[] = { "r", "g", "b", "a" };
static const float sColorDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace Database
{
	Typed<TextureTemplate> texturetemplate(0x3f64431e /* "texturetemplate" */);
	Typed<GLuint> texture(0x3c6468f4 /* "texture" */);

	namespace Loader
	{
		static void TextureConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
				// generate a handle object handle
				GLuint handle;
				glGenTextures( 1, &handle );

				// register the handle
				Database::texture.Put(aId, handle);

				// get a texture template
				TextureTemplate &texture = Database::texturetemplate.Open(handle);

				// set minification filter
				switch (Hash(element->Attribute("minfilter")))
				{
				default:
				case 0xc42bfa19 /* "nearest" */:			texture.mMinFilter = GL_NEAREST; break;
				case 0xd00594c0 /* "linear" */:				texture.mMinFilter = GL_LINEAR; break;
				case 0x70bf16c1 /* "nearestmipnearest" */:	texture.mMinFilter = GL_NEAREST_MIPMAP_NEAREST; break;
				case 0xc81505e8 /* "linearmipnearest" */:	texture.mMinFilter = GL_LINEAR_MIPMAP_NEAREST; break;
				case 0x95d62f98 /* "nearestmiplinear" */:	texture.mMinFilter = GL_NEAREST_MIPMAP_LINEAR; break;
				case 0x1274a447 /* "linearmiplinear" */:	texture.mMinFilter = GL_LINEAR_MIPMAP_LINEAR; break;
				}

				// set magnification filter
				switch (Hash(element->Attribute("magfilter")))
				{
				default:
				case 0xc42bfa19 /* "nearest" */:			texture.mMagFilter = GL_NEAREST; break;
				case 0xd00594c0 /* "linear" */:				texture.mMagFilter = GL_LINEAR; break;
				}

				// set s wrapping
				switch (Hash(element->Attribute("wraps")))
				{
				default:
				case 0xa82efcbc /* "clamp" */:				texture.mWrapS = GL_CLAMP; break;
				case 0xd99ba82a /* "repeat" */:				texture.mWrapS = GL_REPEAT; break;
				}

				// set t wrapping
				switch (Hash(element->Attribute("wrapt")))
				{
				default:
				case 0xa82efcbc /* "clamp" */:				texture.mWrapT = GL_CLAMP; break;
				case 0xd99ba82a /* "repeat" */:				texture.mWrapT = GL_REPEAT; break;
				}

				for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0x7494fdb7 /* "perlin" */:
						{
							texture.mInternalFormat = GL_RGBA8;

							texture.mWidth = 64;
							child->QueryIntAttribute("width", &texture.mWidth);
							texture.mHeight = 64;
							child->QueryIntAttribute("height", &texture.mHeight);

							texture.mFormat = GL_RGBA;

							int octaves = 0;
							child->QueryIntAttribute("octaves", &octaves);

							float frequency = 1.0f;
							child->QueryFloatAttribute("frequency", &frequency);

							float amplitude = 1.0f;
							child->QueryFloatAttribute("amplitude", &amplitude);

							float average = 0.0f;
							child->QueryFloatAttribute("average", &average);

							float persistence = 0.5f;
							child->QueryFloatAttribute("persistence", &persistence);

							float seed = 0.0f;
							child->QueryFloatAttribute("seed", &seed);

							std::vector<unsigned int> buffer;
							Expression::Loader<__m128>::ConfigureRoot(child, buffer, sColorNames, sColorDefault);

							EntityContext context(&buffer[0], buffer.size(), 0, aId);

							texture.Allocate(4);

							unsigned char *pixel = texture.mPixels;

							for (int y = 0; y < texture.mHeight; ++y)
							{
								for (int x = 0; x < texture.mWidth; ++x)
								{
									float value =average;
									float f = frequency;
									float a = amplitude;
									for (int i = 0; i < octaves; ++i)
									{
#if 0
										value += a * Noise(x * f + seed[c], y * f - seed[c], FloorToInt(texture.mWidth * f), FloorToInt(texture.mHeight * f));
#else
										value += a * (
											Noise2D((x) * f, (y) * f) * (texture.mWidth - x) * (texture.mHeight - y) +
											Noise2D((x - texture.mWidth) * f, (y) * f) * (x) * (texture.mHeight - y) +
											Noise2D((x - texture.mWidth) * f, (y - texture.mHeight) * f) * (x) * (y) +
											Noise2D((x) * f, (y - texture.mHeight) * f) * (texture.mWidth - x) * (y)
											) / (texture.mWidth * texture.mHeight);
#endif
										f *= 2;
										a *= persistence;
									}
									value = Clamp(value, -1.0f, 1.0f);

									// get interpolator value
									context.Restart();
									context.mParam = value;
									__m128 color = Expression::Evaluate<__m128>(context);

									// convert to 4 unsigned bytes
#if 1
									__m128i colori = RoundToInt(color * Extend<__m128>(255));		// convert to 4 int32
									colori = _mm_packs_epi32(colori, colori);						// pack to int16
									colori = _mm_packus_epi16(colori, colori);						// pack to uint8
									*reinterpret_cast<uint32_t *>(pixel) = colori.m128i_u32[0];		// copy all 4 bytes at once
									pixel += sizeof(uint32_t);
#elif 1
									color = Clamp01(color) * Extend<__m128>(255);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[0]);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[1]);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[2]);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[3]);
#else
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[0], 0.0f, 1.0f) * 255);
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[1], 0.0f, 1.0f) * 255);
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[2], 0.0f, 1.0f) * 255);
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[3], 0.0f, 1.0f) * 255);
#endif
								}
							}
						}
						break;

					case 0xcf15afeb /* "expression" */:
						{
							texture.mInternalFormat = GL_RGBA8;

							texture.mWidth = 64;
							child->QueryIntAttribute("width", &texture.mWidth);
							texture.mHeight = 64;
							child->QueryIntAttribute("height", &texture.mHeight);

							texture.mFormat = GL_RGBA;

							std::vector<unsigned int> buffer;
							Expression::Loader<__m128>::ConfigureRoot(child, buffer, sColorNames, sColorDefault);

							EntityContext context(&buffer[0], buffer.size(), 0, aId);

							texture.mPixels = static_cast<unsigned char *>(malloc(texture.mWidth * texture.mHeight * 4));

							unsigned char *pixel = texture.mPixels;

							for (int y = 0; y < texture.mHeight; ++y)
							{
								context.mVars->Put(0xfc0c4ef4 /* "y" */, float(y)/float(texture.mHeight));

								for (int x = 0; x < texture.mWidth; ++x)
								{
									context.mVars->Put(0xfd0c5087 /* "x" */, float(x)/float(texture.mWidth));

									// get interpolator value
									context.Restart();
									__m128 color = Expression::Evaluate<__m128>(context);

									// convert to 4 unsigned bytes
#if 1
									__m128i colori = RoundToInt(color * Extend<__m128>(255));		// convert to 4 int32
									colori = _mm_packs_epi32(colori, colori);						// pack to int16
									colori = _mm_packus_epi16(colori, colori);						// pack to uint8
									*reinterpret_cast<uint32_t *>(pixel) = colori.m128i_u32[0];		// copy all 4 bytes at once
									pixel += sizeof(uint32_t);
#elif 1
									color = Clamp01(color) * Extend<__m128>(255);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[0]);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[1]);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[2]);
									*pixel++ = (unsigned char)RoundToInt(color.m128_f32[3]);
#else
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[0], 0.0f, 1.0f) * 255);
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[1], 0.0f, 1.0f) * 255);
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[2], 0.0f, 1.0f) * 255);
									*pixel++ = (unsigned char)RoundToInt(Clamp(reinterpret_cast<float * __restrict>(&color)[3], 0.0f, 1.0f) * 255);
#endif
								}
							}

							context.mVars->Delete(0xfd0c5087 /* "x" */);
							context.mVars->Delete(0xfc0c4ef4 /* "y" */);
						}
						break;

					case 0xaaea5743 /* "file" */:
						if (const char *file = child->Attribute("name"))
						{
							if (!Platform::LoadTexture(texture, file))
							{
								DebugPrint("error: could not open %s\n", file);
								continue;
							}
						}
						break;
					}
				}

				// bind the texture
				BindTexture(handle, texture);

				// done with texture template
				Database::texturetemplate.Close(handle);
			}
		Configure textureconfigure(0x3c6468f4 /* "texture" */, TextureConfigure);
		}
}

void BindTexture(GLuint handle, TextureTemplate const &texture)
{
	// save texture state
	glPushAttrib(GL_TEXTURE_BIT);

	// bind the texture object
	glBindTexture(GL_TEXTURE_2D, handle);

	// set texture properties
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.mMinFilter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.mMagFilter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture.mWrapS );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture.mWrapT );
	//glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE ); 

	// set texture image data
	if (texture.mMipmaps)
		gluBuild2DMipmaps(GL_TEXTURE_2D, texture.mInternalFormat, texture.mWidth, texture.mHeight, texture.mFormat, GL_UNSIGNED_BYTE, texture.mPixels);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, texture.mInternalFormat, texture.mWidth, texture.mHeight, 0, texture.mFormat, GL_UNSIGNED_BYTE, texture.mPixels);

	// restore texture state
	glPopAttrib();
}

void CleanupTextures(void)
{
	// for each entry in the texture database
	for (Database::Typed<GLuint>::Iterator itor(&Database::texture); itor.IsValid(); ++itor)
	{
		// delete the texture
		GLuint handle = itor.GetValue();
		if (glIsTexture(handle))
			glDeleteTextures(1, &handle);
	}
}

void RebuildTextures(void)
{
	// for each entry in the texture database
	for (Database::Typed<GLuint>::Iterator itor(&Database::texture); itor.IsValid(); ++itor)
	{
		// recreate the texture
		GLuint handle = itor.GetValue();

		// get the corresponding template
		const TextureTemplate &texture = Database::texturetemplate.Get(handle);
		if (texture.mWidth == 0 || texture.mHeight == 0)
			continue;

		// bind the texture
		BindTexture(handle, texture);
	}
}
