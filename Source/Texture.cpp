#include "StdAfx.h"

#include "Texture.h"
#include "Interpolator.h"
#include "Noise.h"

static const char * sColorNames[] = { "r", "g", "b", "a" };
static const float sColorDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace Database
{
	Typed<TextureTemplate> texturetemplate(0x3f64431e /* "texturetemplate" */);
	Typed<GLuint> texture(0x3c6468f4 /* "texture" */);

	namespace Loader
	{
		class TextureLoader
		{
		public:
			TextureLoader()
			{
				AddConfigure(0x3c6468f4 /* "texture" */, Entry(this, &TextureLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// generate a handle object handle
				GLuint handle;
				glGenTextures( 1, &handle );

				// register the handle
				Database::texture.Put(aId, handle);

				// get a texture template
				TextureTemplate &texture = Database::texturetemplate.Open(handle);

				// set blend mode
				switch (Hash(element->Attribute("mode")))
				{
				default:
				case 0x818f75ae /* "modulate" */:	texture.mEnvMode = GL_MODULATE; break;
				case 0xde15f6ae /* "decal" */:		texture.mEnvMode = GL_DECAL; break;
				case 0x0bbc40d8 /* "blend" */:		texture.mEnvMode = GL_BLEND; break;
				case 0xa13884c3 /* "replace" */:	texture.mEnvMode = GL_REPLACE; break;
				}

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

				// save texture state
				glPushAttrib(GL_TEXTURE_BIT);

				// bind the texture object
				glBindTexture(GL_TEXTURE_2D, handle);

				// set texture properties
				glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texture.mEnvMode );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.mMinFilter );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.mMagFilter );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture.mWrapS );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture.mWrapT );

				for (const TiXmlElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0x7494fdb7 /* "perlin" */:
						{
							texture.mComponents = 4;

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
							ConfigureInterpolatorItem(child, buffer, SDL_arraysize(sColorNames), sColorNames, sColorDefault);

							texture.mPixels = static_cast<unsigned char *>(malloc(texture.mWidth * texture.mHeight * texture.mComponents));

							unsigned char *pixel = texture.mPixels;

							// interpolator output
							int index = 0;
							float data[SDL_arraysize(sColorNames)];

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
										value += a * Noise(x * f + seed[c], y * f - seed[c], xs_FloorToInt(texture.mWidth * f), xs_FloorToInt(texture.mHeight * f));
#else
										value += a * (
											Noise((x) * f, (y) * f) * (texture.mWidth - x) * (texture.mHeight - y) +
											Noise((x - texture.mWidth) * f, (y) * f) * (x) * (texture.mHeight - y) +
											Noise((x - texture.mWidth) * f, (y - texture.mHeight) * f) * (x) * (y) +
											Noise((x) * f, (y - texture.mHeight) * f) * (texture.mWidth - x) * (y)
											) / (texture.mWidth * texture.mHeight);
#endif
										f *= 2;
										a *= persistence;
									}
									value = Clamp(value, -1.0f, 1.0f);

									// get interpolator value
									if (!ApplyInterpolator(data, SDL_arraysize(data), buffer[0], reinterpret_cast<const float * __restrict>(&buffer[1]), value, index))
										memset(data, 0, sizeof(data));

									*pixel++ = (unsigned char)xs_RoundToInt(data[0] * 255);
									*pixel++ = (unsigned char)xs_RoundToInt(data[1] * 255);
									*pixel++ = (unsigned char)xs_RoundToInt(data[2] * 255);
									*pixel++ = (unsigned char)xs_RoundToInt(data[3] * 255);
								}
							}
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
					}
				}

				// set texture image data
				gluBuild2DMipmaps(GL_TEXTURE_2D, texture.mComponents, texture.mWidth, texture.mHeight, texture.mFormat, GL_UNSIGNED_BYTE, texture.mPixels);

				// done with texture template
				Database::texturetemplate.Close(handle);

				// restore texture state
				glPopAttrib();
			}
		}
		textureloader;

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
		if (!texture.mPixels)
			continue;

		// save texture state
		glPushAttrib(GL_TEXTURE_BIT);

		// bind the texture object
		glBindTexture(GL_TEXTURE_2D, handle);

		// set texture properties
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texture.mEnvMode );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.mMinFilter );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.mMagFilter );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture.mWrapS );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture.mWrapT );

		// set texture image data
		gluBuild2DMipmaps(GL_TEXTURE_2D, texture.mComponents, texture.mWidth, texture.mHeight, texture.mFormat, GL_UNSIGNED_BYTE, texture.mPixels);

		/*
		glTexImage2D(
			GL_TEXTURE_2D, 0, texture.mSurface->format->BytesPerPixel, texture.mSurface->w, texture.mSurface->h, 0,
			texture.mFormat, GL_UNSIGNED_BYTE, texture.mSurface->pixels
			);
		*/

		// restore texture state
		glPopAttrib();
	}
}
