#include "StdAfx.h"
#include "Interpolator.h"
#include "Drawlist.h"

namespace Database
{
	Typed<std::vector<unsigned int> > dynamicdrawlist(0xdf3cf9c0 /* "dynamicdrawlist" */);
	Typed<GLuint> drawlist(0xc98b019b /* "drawlist" */);
	Typed<GLuint> texture(0x3c6468f4 /* "texture" */);

	namespace Loader
	{
		class DynamicDrawlistLoader
		{
		public:
			DynamicDrawlistLoader()
			{
				AddConfigure(0xdf3cf9c0 /* "dynamicdrawlist" */, Entry(this, &DynamicDrawlistLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Open(aId);
				ProcessDrawItems(element, buffer);
				Database::dynamicdrawlist.Close(aId);
			}
		}
		dynamicdrawlistloader;

		class DrawlistLoader
		{
		public:
			DrawlistLoader()
			{
				AddConfigure(0xc98b019b /* "drawlist" */, Entry(this, &DrawlistLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// create a new draw list
				GLuint handle = glGenLists(1);
				glNewList(handle, GL_COMPILE);

				// register the draw list
				Database::drawlist.Put(aId, handle);

				// get (optional) parameter value
				float param = 0.0f;
				element->QueryFloatAttribute("param", &param);

				// process draw items
				std::vector<unsigned int> drawlist;
				ProcessDrawItems(element, drawlist);
				ExecuteDrawItems(&drawlist[0], drawlist.size(), param);

				// finish the draw list
				glEndList();

				// finish the draw list
				glEndList();
			}
		}
		drawlistloader;

		class TextureLoader
		{
		public:
			TextureLoader()
			{
				AddConfigure(0x3c6468f4 /* "texture" */, Entry(this, &TextureLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				for (const TiXmlElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0xaaea5743 /* "file" */:
						if (const char *file = child->Attribute("name"))
						{
							// get the surface
							SDL_Surface *surface = SDL_LoadBMP(file);
							if (!surface)
								continue;

							// check if the width is a power of 2
							if ((surface->w & (surface->w - 1)) != 0)
							{
								DebugPrint("warning: %s width %d is not a power of 2\n", file, surface->w);
							}
							
							// check if the height is a power of 2
							if ((surface->h & (surface->h - 1)) != 0)
							{
								DebugPrint("warning: %s height %d is not a power of 2\n", file, surface->h);
							}
						 
							// get the number of channels in the SDL surface
							GLenum texture_format;
							GLint color_size = surface->format->BytesPerPixel;
							if (color_size == 4)     // contains an alpha channel
							{
								if (surface->format->Rmask == 0x000000ff)
									texture_format = GL_RGBA;
								else
									texture_format = GL_BGRA;
							}
							else if (color_size == 3)     // no alpha channel
							{
								if (surface->format->Rmask == 0x000000ff)
									texture_format = GL_RGB;
								else
									texture_format = GL_BGR;
							}
							else
							{
								DebugPrint("warning: %s is not truecolor\n", file);
								SDL_FreeSurface( surface );
								break;
							}

							// generate a texture object handle
							GLuint texture;
							glGenTextures( 1, &texture );

							// register the texture
							Database::texture.Put(aId, texture);

							// save texture state
							glPushAttrib(GL_TEXTURE_BIT);

							// bind the texture object
							glBindTexture(GL_TEXTURE_2D, texture);

							// mode
							GLint mode;

							// set blend mode
							switch (Hash(child->Attribute("mode")))
							{
							default:
							case 0x818f75ae /* "modulate" */:	mode = GL_MODULATE; break;
							case 0xde15f6ae /* "decal" */:		mode = GL_DECAL; break;
							case 0x0bbc40d8 /* "blend" */:		mode = GL_BLEND; break;
							case 0xa13884c3 /* "replace" */:	mode = GL_REPLACE; break;
							}
							glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );

							// set minification filter
							switch (Hash(child->Attribute("minfilter")))
							{
							default:
							case 0xc42bfa19 /* "nearest" */:			mode = GL_NEAREST; break;
							case 0xd00594c0 /* "linear" */:				mode = GL_LINEAR; break;
							case 0x70bf16c1 /* "nearestmipnearest" */:	mode = GL_NEAREST_MIPMAP_NEAREST; break;
							case 0xc81505e8 /* "linearmipnearest" */:	mode = GL_LINEAR_MIPMAP_NEAREST; break;
							case 0x95d62f98 /* "nearestmiplinear" */:	mode = GL_NEAREST_MIPMAP_LINEAR; break;
							case 0x1274a447 /* "linearmiplinear" */:	mode = GL_LINEAR_MIPMAP_LINEAR; break;
							}
							glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode );

							// set magnification filter
							switch (Hash(child->Attribute("magfilter")))
							{
							default:
							case 0xc42bfa19 /* "nearest" */:			mode = GL_NEAREST; break;
							case 0xd00594c0 /* "linear" */:				mode = GL_LINEAR; break;
							}
							glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode );

							// set s wrapping
							switch (Hash(child->Attribute("wraps")))
							{
							default:
							case 0xa82efcbc /* "clamp" */:				mode = GL_CLAMP; break;
							case 0xd99ba82a /* "repeat" */:				mode = GL_REPEAT; break;
							}
							glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode );

							// set t wrapping
							switch (Hash(child->Attribute("wrapt")))
							{
							default:
							case 0xa82efcbc /* "clamp" */:				mode = GL_CLAMP; break;
							case 0xd99ba82a /* "repeat" */:				mode = GL_REPEAT; break;
							}
							glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode );

							// set texture image data
							glTexImage2D(
								GL_TEXTURE_2D, 0, color_size, surface->w, surface->h, 0,
								texture_format, GL_UNSIGNED_BYTE, surface->pixels
								);

							// restore texture state
							glPopAttrib();

							// free the surface
							SDL_FreeSurface( surface );
						}
					}
				}
			}
		}
		textureloader;
	}

}

#if 0
static const unsigned int sHashToAttribMask[][2] =
{
	{ 0xd965bbda /* "current" */,			GL_CURRENT_BIT },
	{ 0x18ae6c91 /* "point" */,				GL_POINT_BIT },
	{ 0x17db1627 /* "line" */,				GL_LINE_BIT },
	{ 0x051cb889 /* "polygon" */,			GL_POLYGON_BIT },
	{ 0x67b14997 /* "polygon_stipple" */,	GL_POLYGON_STIPPLE_BIT },
	{ 0xccde91eb /* "pixel_mode" */,		GL_LIGHTING_BIT },
	{ 0x827eb1c9 /* "lighting" */,			GL_POINT_BIT },
	{ 0xa1f3723f /* "fog" */,				GL_FOG_BIT },
	{ 0x65e5b825 /* "depth_buffer" */,		GL_DEPTH_BUFFER_BIT },
	{ 0x907f6213 /* "accum_buffer" */,		GL_ACCUM_BUFFER_BIT },
	{ 0x632020be /* "stencil_buffer" */,	GL_STENCIL_BUFFER_BIT },
	{ 0xe4abbac3 /* "viewport" */,			GL_VIEWPORT_BIT },
	{ 0xe1ad931b /* "transform" */,			GL_TRANSFORM_BIT },
	{ 0xaf8bb8ce /* "enable" */,			GL_ENABLE_BIT },
	{ 0x0d759bbb /* "color_buffer" */,		GL_COLOR_BUFFER_BIT },
	{ 0x4bc809b8 /* "hint" */,				GL_HINT_BIT },
	{ 0x08d22e0f /* "eval" */,				GL_EVAL_BIT },
	{ 0x0cfb5881 /* "list" */,				GL_LIST_BIT },
	{ 0x3c6468f4 /* "texture" */,			GL_TEXTURE_BIT },
	{ 0x0adbc081 /* "scissor" */,			GL_SCISSOR_BIT },
};
#endif

// attribute names
static const char * sPositionNames[] = { "x", "y", "z", "w" };
static const char * sRotationNames[] = { "angle", "x", "y", "z" };
static const char * sColorNames[] = { "r", "g", "b", "a" };
static const char * sTexCoordNames[] = { "s", "t", "r", "q" };
static const char * sIndexNames[] = { "c" };
static const char * sMatrixNames[] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15" };

void ProcessDrawData(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char *names[], const float data[])
{
	if (const char *name = element->Attribute("name"))
	{
		// push a reference to a variable value
		buffer.push_back(0x19385305 /* "variable" */);
		buffer.push_back(Hash(name));
	}
	else if (element->FirstChildElement())
	{
		// push an interpolator
		buffer.push_back(0x83588fd4 /* "interpolator" */);
		buffer.push_back(0);
		int start = buffer.size();
		ProcessInterpolatorItem(element, buffer, width, names, data);
		buffer[start - 1] = buffer.size() - start;
	}
	else
	{
		// push literal data
		buffer.push_back(0xecb9d8e4 /* "literal" */);
		for (int i = 0; i < width; i++)
		{
			float value = data[i];
			element->QueryFloatAttribute(names[i], &value);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
		}
	}
}

void ProcessDrawItem(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x974c9474 /* "pushmatrix" */:
		{
			buffer.push_back(0xf6604733 /* "glPushMatrix" */);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0xfc8a1d94 /* "glPopMatrix" */);
		}
		break;

	case 0x937cff81 /* "pushattrib" */:
		{
			GLuint mask = 0U;
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				switch (Hash(attrib->Name()))
				{
				case 0xd965bbda /* "current" */:
					if (attrib->IntValue())
						mask |= GL_CURRENT_BIT;
					else
						mask &= ~GL_CURRENT_BIT;
					break;
				case 0x18ae6c91 /* "point" */:
					if (attrib->IntValue())
						mask |= GL_POINT_BIT;
					else
						mask &= ~GL_POINT_BIT;
					break;
				case 0x17db1627 /* "line" */:
					if (attrib->IntValue())
						mask |= GL_LINE_BIT;
					else
						mask &= ~GL_LINE_BIT;
					break;
				case 0x051cb889 /* "polygon" */:
					if (attrib->IntValue())
						mask |= GL_POLYGON_BIT;
					else
						mask &= ~GL_POLYGON_BIT;
					break;
				case 0x67b14997 /* "polygon_stipple" */:
					if (attrib->IntValue())
						mask |= GL_POLYGON_STIPPLE_BIT;
					else
						mask &= ~GL_POLYGON_STIPPLE_BIT;
					break;
				case 0xccde91eb /* "pixel_mode" */:
					if (attrib->IntValue())
						mask |= GL_LIGHTING_BIT;
					else
						mask &= ~GL_LIGHTING_BIT;
					break;
				case 0x827eb1c9 /* "lighting" */:
					if (attrib->IntValue())
						mask |= GL_POINT_BIT;
					else
						mask &= ~GL_POINT_BIT;
					break;
				case 0xa1f3723f /* "fog" */:
					if (attrib->IntValue())
						mask |= GL_FOG_BIT;
					else
						mask &= ~GL_FOG_BIT;
					break;
				case 0x65e5b825 /* "depth_buffer" */:
					if (attrib->IntValue())
						mask |= GL_DEPTH_BUFFER_BIT;
					else
						mask &= ~GL_DEPTH_BUFFER_BIT;
					break;
				case 0x907f6213 /* "accum_buffer" */:
					if (attrib->IntValue())
						mask |= GL_ACCUM_BUFFER_BIT;
					else
						mask &= ~GL_ACCUM_BUFFER_BIT;
					break;
				case 0x632020be /* "stencil_buffer" */:
					if (attrib->IntValue())
						mask |= GL_STENCIL_BUFFER_BIT;
					else
						mask &= ~GL_STENCIL_BUFFER_BIT;
					break;
				case 0xe4abbac3 /* "viewport" */:
					if (attrib->IntValue())
						mask |= GL_VIEWPORT_BIT;
					else
						mask &= ~GL_VIEWPORT_BIT;
					break;
				case 0xe1ad931b /* "transform" */:
					if (attrib->IntValue())
						mask |= GL_TRANSFORM_BIT;
					else
						mask &= ~GL_TRANSFORM_BIT;
					break;
				case 0xaf8bb8ce /* "enable" */:
					if (attrib->IntValue())
						mask |= GL_ENABLE_BIT;
					else
						mask &= ~GL_ENABLE_BIT;
					break;
				case 0x0d759bbb /* "color_buffer" */:
					if (attrib->IntValue())
						mask |= GL_COLOR_BUFFER_BIT;
					else
						mask &= ~GL_COLOR_BUFFER_BIT;
					break;
				case 0x4bc809b8 /* "hint" */:
					if (attrib->IntValue())
						mask |= GL_HINT_BIT;
					else
						mask &= ~GL_HINT_BIT;
					break;
				case 0x08d22e0f /* "eval" */:
					if (attrib->IntValue())
						mask |= GL_EVAL_BIT;
					else
						mask &= ~GL_EVAL_BIT;
					break;
				case 0x0cfb5881 /* "list" */:
					if (attrib->IntValue())
						mask |= GL_LIST_BIT;
					else
						mask &= ~GL_LIST_BIT;
					break;
				case 0x3c6468f4 /* "texture" */:
					if (attrib->IntValue())
						mask |= GL_TEXTURE_BIT;
					else
						mask &= ~GL_TEXTURE_BIT;
					break;
				case 0x0adbc081 /* "scissor" */:
					if (attrib->IntValue())
						mask |= GL_SCISSOR_BIT;
					else
						mask &= ~GL_SCISSOR_BIT;
					break;
				}
			}
			buffer.push_back(0xa471ec02 /* "glPushAttrib" */);
			buffer.push_back(mask);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x73c4cda1 /* "glPopAttrib" */);
		}
		break;

	case 0x052eb8b2 /* "pushclientattrib" */:
		{
			GLuint mask = 0U;
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				switch (Hash(attrib->Name()))
				{
				case 0x959fee19 /* "pixel_store" */:
					if (attrib->IntValue())
						mask |= GL_CLIENT_PIXEL_STORE_BIT;
					else
						mask &= ~GL_CLIENT_PIXEL_STORE_BIT;
					break;
				case 0x20a16825 /* "vertex_array" */:
					if (attrib->IntValue())
						mask |= GL_CLIENT_VERTEX_ARRAY_BIT;
					else
						mask &= ~GL_CLIENT_VERTEX_ARRAY_BIT;
					break;
				}
			}
			buffer.push_back(0x485249b9 /* "glPushClientAttrib" */);
			buffer.push_back(mask);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0xbfd4add2 /* "glPopClientAttrib" */);
		}
		break;

	case 0xad0ecfd5 /* "translate" */:
		{
			float data[3] = { 0.0f, 0.0f, 0.0f };
			buffer.push_back(0xafeef11e /* "glTranslatef" */);
			ProcessDrawData(element, buffer, 3, sPositionNames, data);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0x29e02ba1 /* "glRotatef" */);
			ProcessDrawData(element, buffer, 4, sRotationNames, data);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			float data[3] = { 1.0f, 1.0f, 1.0f };
			buffer.push_back(0xff71cf6e /* "glScalef" */);
			ProcessDrawData(element, buffer, 3, sPositionNames, data);
		}
		break;

	case 0x938fc4f7 /* "loadidentity" */:
		{
			buffer.push_back(0xcbd6bd5c /* "glLoadIdentity" */);
		}
		break;

	case 0x7d22a710 /* "loadmatrix" */:
		{
			GLfloat m[16] = {
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			};
			buffer.push_back(0xca9090d7 /* "glLoadMatrixf" */);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				element->QueryFloatAttribute(name, &m[i]);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m[i]));
			}
		}
		break;

	case 0x3807cb92 /* "multmatrix" */:
		{
			GLfloat m[16] = {
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			};
			buffer.push_back(0x64500671 /* "glMultMatrixf" */);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				element->QueryFloatAttribute(name, &m[i]);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m[i]));
			}
		}
		break;

	case 0x945367a7 /* "vertex" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0x94110c7a /* "glVertex4f" */);
			ProcessDrawData(element, buffer, 4, sPositionNames, data);
		}
		break;
	case 0xe68b9c52 /* "normal" */:
		{
			float data[3] = { 0.0f, 0.0f, 0.0f };
			buffer.push_back(0xf2d58094 /* "glNormal3f" */);
			ProcessDrawData(element, buffer, 4, sPositionNames, data);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0x9d63d16b /* "glColor4f" */);
			ProcessDrawData(element, buffer, 4, sColorNames, data);
		}
		break;

	case 0x090aa9ab /* "index" */:
		{
			float data[1] = { 0.0f };
			buffer.push_back(0xf3b3b82c /* "glIndexf" */);
			ProcessDrawData(element, buffer, 1, sIndexNames, data);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0xb78bb2ae /* "glTexCoord4f" */);
			ProcessDrawData(element, buffer, 4, sTexCoordNames, data);
		}
		break;

	case 0x0135ab46 /* "edgeflag" */:
		{
			int flag;
			if (element->QueryIntAttribute("flag", &flag) == TIXML_SUCCESS)
			{
				buffer.push_back(0x7f5dcd49 /* "glEdgeFlag" */);
				buffer.push_back(flag ? GL_TRUE : GL_FALSE);
			}
		}
		break;

	case 0x4dead571 /* "bindtexture" */:
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				GLuint texture = Database::texture.Get(Hash(name));
				if (texture)
				{
					// bind the texture object
					buffer.push_back(0x2ed38a3d /* "glEnable" */);
					buffer.push_back(GL_TEXTURE_2D);
					buffer.push_back(0x51956b0c /* "glBindTexture" */);
					buffer.push_back(GL_TEXTURE_2D);
					buffer.push_back(texture);
				}
			}
		}
		break;

	case 0xbc9567c6 /* "points" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_POINTS);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_LINES);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_LINE_LOOP);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_LINE_STRIP);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xd8a57342 /* "triangles" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_TRIANGLES);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_TRIANGLE_STRIP);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_TRIANGLE_FAN);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_QUADS);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_QUAD_STRIP);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_POLYGON);
			ProcessDrawItems(element, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xd2cf6b75 /* "calllist" */:
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				GLuint drawlist = Database::drawlist.Get(Hash(name));
				if (drawlist)
				{
					buffer.push_back(0x9525d6fe /* "glCallList" */);
					buffer.push_back(drawlist);
				}
			}
		}
		break;

	case 0xc98b019b /* "drawlist" */:
		{
			// create a new draw list
			GLuint handle = glGenLists(1);
			glNewList(handle, GL_COMPILE);

			// get (optional) parameter value
			float param = 0.0f;
			element->QueryFloatAttribute("param", &param);

			// process draw items
			std::vector<unsigned int> drawlist;
			ProcessDrawItems(element, drawlist);
			ExecuteDrawItems(&drawlist[0], drawlist.size(), param);

			// finish the draw list
			glEndList();

			// use the anonymous drawlist
			buffer.push_back(0x9525d6fe /* "glCallList" */);
			buffer.push_back(handle);
		}
		break;

	case 0x2610a4a3 /* "clientstate" */:
		{
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				switch (Hash(attrib->Name()))
				{
				case 0x945367a7 /* "vertex" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_VERTEX_ARRAY);
					break;
				case 0xe68b9c52 /* "normal" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_NORMAL_ARRAY);
					break;
				case 0x3d7e6258 /* "color" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_COLOR_ARRAY);
					break;
				case 0x090aa9ab /* "index" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_INDEX_ARRAY);
					break;
				case 0xdd612dd3 /* "texcoord" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_TEXTURE_COORD_ARRAY);
					break;
				case 0x0135ab46 /* "edgeflag" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_EDGE_FLAG_ARRAY);
					break;
				}
			}
		}
		break;

	case 0x6298bce4 /* "vertexarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				data[count++] = float(atof(element));
				element = strtok(NULL, " \t\n\r,;");
			}

			buffer.push_back(0x4e467465 /* "glVertexPointer" */);
			buffer.push_back(size);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x81491d33 /* "normalarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				data[count++] = float(atof(element));
				element = strtok(NULL, " \t\n\r,;");
			}

			buffer.push_back(0x46804012 /* "glNormalPointer" */);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0xcce5b995 /* "colorarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				data[count++] = float(atof(element));
				element = strtok(NULL, " \t\n\r,;");
			}

			buffer.push_back(0x61e8560e /* "glColorPointer" */);
			buffer.push_back(size);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x664ead80 /* "indexarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				data[count++] = float(atof(element));
				element = strtok(NULL, " \t\n\r,;");
			}

			buffer.push_back(0x1e5cf423 /* "glIndexPointer" */);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x91aa3148 /* "texcoordarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				data[count++] = float(atof(element));
				element = strtok(NULL, " \t\n\r,;");
			}

			buffer.push_back(0x6d976421 /* "glTexCoordPointer" */);
			buffer.push_back(size);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x60360ccf /* "edgeflagarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			bool *data = static_cast<bool *>(_alloca(len*sizeof(bool)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				data[count++] = atoi(element) != 0;
				element = strtok(NULL, " \t\n\r,;");
			}

			buffer.push_back(0x9cfbc596 /* "glEdgeFlagPointer" */);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (size_t i = 0; i < (count+sizeof(unsigned int)/sizeof(bool)-1)/(sizeof(unsigned int)/sizeof(bool)); i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i*sizeof(unsigned int)/sizeof(bool)]));
		}
		break;

	case 0x0a85bb5e /* "arrayelement" */:
		{
			int index;
			if (element->QueryIntAttribute("index", &index) == TIXML_SUCCESS)
			{
				buffer.push_back(0x8cfc8329 /* "glArrayElement" */);
				buffer.push_back(index);
			}
		}
		break;

	case 0xf4de4a21 /* "drawarrays" */:
		{
			GLenum mode;
			switch (Hash(element->Attribute("mode")))
			{
			case 0xbc9567c6 /* "points" */:			mode = GL_POINTS; break;
			case 0xe1e4263c /* "lines" */:			mode = GL_LINES; break;
			case 0xc2106ab6 /* "line_loop" */:		mode = GL_LINE_LOOP; break;
			case 0xc6f2fa0e /* "line_strip" */:		mode = GL_LINE_STRIP; break;
			case 0xd8a57342 /* "triangles" */:		mode = GL_TRIANGLES; break;
			case 0x668b2dd8 /* "triangle_strip" */:	mode = GL_TRIANGLE_STRIP; break;
			case 0xcfa6904f /* "triangle_fan" */:	mode = GL_TRIANGLE_FAN; break;
			case 0x5667b307 /* "quads" */:			mode = GL_QUADS; break;
			case 0xb47cad9b /* "quad_strip" */:		mode = GL_QUAD_STRIP; break;
			case 0x051cb889 /* "polygon" */:		mode = GL_POLYGON; break;
			default: break;
			}

			int first = 0, count = 0;
			element->QueryIntAttribute("first", &first);
			element->QueryIntAttribute("count", &count);
			buffer.push_back(0x806f1b62 /* "glDrawArrays" */);
			buffer.push_back(mode);
			buffer.push_back(first);
			buffer.push_back(count);
		}
		break;

	case 0x757eeee2 /* "drawelements" */:
		{
			GLenum mode;
			switch (Hash(element->Attribute("mode")))
			{
			case 0xbc9567c6 /* "points" */:			mode = GL_POINTS; break;
			case 0xe1e4263c /* "lines" */:			mode = GL_LINES; break;
			case 0xc2106ab6 /* "line_loop" */:		mode = GL_LINE_LOOP; break;
			case 0xc6f2fa0e /* "line_strip" */:		mode = GL_LINE_STRIP; break;
			case 0xd8a57342 /* "triangles" */:		mode = GL_TRIANGLES; break;
			case 0x668b2dd8 /* "triangle_strip" */:	mode = GL_TRIANGLE_STRIP; break;
			case 0xcfa6904f /* "triangle_fan" */:	mode = GL_TRIANGLE_FAN; break;
			case 0x5667b307 /* "quads" */:			mode = GL_QUADS; break;
			case 0xb47cad9b /* "quad_strip" */:		mode = GL_QUAD_STRIP; break;
			case 0x051cb889 /* "polygon" */:		mode = GL_POLYGON; break;
			default: break;
			}

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			unsigned short *indices = static_cast<unsigned short *>(_alloca(len*sizeof(unsigned short)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				indices[count++] = unsigned short(atoi(element));
				element = strtok(NULL, " \t\n\r,;");
			}

			buffer.push_back(0xf6e885d9 /* "glDrawElements" */);
			buffer.push_back(mode);
			buffer.push_back(count);
			for (size_t i = 0; i < (count+sizeof(unsigned int)/sizeof(unsigned short)-1)/(sizeof(unsigned int)/sizeof(unsigned short)); i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&indices[i*sizeof(unsigned int)/sizeof(unsigned short)]));
		}
		break;

	case 0xd99ba82a /* "repeat" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);
			buffer.push_back(0xd99ba82a /* "repeat" */);
			buffer.push_back(count);
			buffer.push_back(0);
			int start = buffer.size();
			ProcessDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;

	default:
		break;
	}
}

void ProcessDrawItems(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessDrawItem(child, buffer);
	}
}

size_t ExecuteDrawData(const unsigned int buffer[], size_t count, int width, float data[], float param)
{
	int index = 0;
	switch(buffer[index++])
	{
	case 0x19385305 /* "variable" */:
		{
			// TO DO: get the global value
			unsigned int name = buffer[index++];
		}
		break;

	case 0x83588fd4 /* "interpolator" */:
		{
			unsigned int size = buffer[index++];

			// get interpolator value
			InterpolatorTemplate interpolator(width);
			interpolator.mCount = buffer[index];
			interpolator.mKeys = (float *)&buffer[index+1];
			int dummy = 0;
			if (!interpolator.Apply(data, param, dummy))
				memset(data, 0, width*sizeof(float));
			interpolator.mKeys = NULL;

			index += size;
		}
		break;

	case 0xecb9d8e4 /* "literal" */:
		{
			// get literal value
			for (int i = 0; i < width; i++)
				data[i] = *reinterpret_cast<const float *>(&buffer[index++]);
		}
		break;

	default:
		break;
	}

	return index;
}

void ExecuteDrawItems(const unsigned int buffer[], size_t count, float param)
{
	GLfloat data[4];

	const unsigned int *itor = buffer;
	while (itor < buffer + count)
	{
		switch (*itor++)
		{
		case 0x2ed38a3d /* "glEnable" */:
			glEnable(*itor++);
			break;

		case 0xbc4dc976 /* "glDisable" */:
			glDisable(*itor++);
			break;

		case 0x51956b0c /* "glBindTexture" */:
			glBindTexture(itor[0], itor[1]);
			itor += 2;
			break;

		case 0xf6604733 /* "glPushMatrix" */:
			glPushMatrix();
			break;

		case 0xfc8a1d94 /* "glPopMatrix" */:
			glPopMatrix();
			break;

		case 0xa471ec02 /* "glPushAttrib" */:
			glPushAttrib(GLbitfield(*itor++));
			break;

		case 0x73c4cda1 /* "glPopAttrib" */:
			glPopAttrib();
			break;

		case 0x485249b9 /* "glPushClientAttrib" */:
			glPushClientAttrib(GLbitfield(*itor++));
			break;

		case 0xbfd4add2 /* "glPopClientAttrib" */:
			glPopClientAttrib();
			break;

		case 0xafeef11e /* "glTranslatef" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 3, data, param);
			glTranslatef(data[0], data[1], data[2]);
			break;

		case 0x29e02ba1 /* "glRotatef" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param);
			glRotatef(data[0], data[1], data[2], data[3]);
			break;

		case 0xff71cf6e /* "glScalef" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 3, data, param);
			glScalef(data[0], data[1], data[2]);
			break;

		case 0xcbd6bd5c /* "glLoadIdentity" */:
			glLoadIdentity();
			break;

		case 0xca9090d7 /* "glLoadMatrixf" */:
			glLoadMatrixf(reinterpret_cast<const GLfloat *>(&*itor));
			itor+=16;
			break;

		case 0x64500671 /* "glMultMatrixf" */:
			glMultMatrixf(reinterpret_cast<const GLfloat *>(&*itor));
			itor+=16;
			break;

		case 0x94110c7a /* "glVertex4f" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param);
			glVertex4fv(data);
			break;

		case 0xf2d58094 /* "glNormal3f" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 3, data, param);
			glNormal3fv(data);
			break;

		case 0x9d63d16b /* "glColor4f" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param);
			glColor4fv(data);
			break;

		case 0xf3b3b82c /* "glIndexf" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 1, data, param);
			glIndexf(data[0]);
			break;

		case 0xb78bb2ae /* "glTexCoord4f" */:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param);
			glTexCoord4fv(data);
			break;

		case 0x7f5dcd49 /* "glEdgeFlag" */:
			glEdgeFlag(*itor++ != 0);
			break;

		case 0xb70e76e3 /* "glBegin" */:
			glBegin(GLenum(*itor++));
			break;

		case 0x50257afb /* "glEnd" */:
			glEnd();
			break;

		case 0x9525d6fe /* "glCallList" */:
			glCallList(GLuint(*itor++));
			break;

		case 0x9128677b /* "glEnableClientState" */:
			glEnableClientState(GLenum(*itor++));
			break;

		case 0x342d0316 /* "glDisableClientState" */:
			glDisableClientState(GLenum(*itor++));
			break;

		case 0x4e467465 /* "glVertexPointer" */:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glVertexPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x46804012 /* "glNormalPointer" */:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glNormalPointer(GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x61e8560e /* "glColorPointer" */:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glColorPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x1e5cf423 /* "glIndexPointer" */:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glIndexPointer(GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x6d976421 /* "glTexCoordPointer" */:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glTexCoordPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x9cfbc596 /* "glEdgeFlagPointer" */:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glEdgeFlagPointer(stride, &*itor);
				itor += count*sizeof(bool)/sizeof(unsigned int);
			}
			break;

		case 0x8cfc8329 /* "glArrayElement" */:
			glArrayElement(*itor++);
			break;

		case 0x806f1b62 /* "glDrawArrays" */:
			{
				GLenum mode = *itor++;
				GLint first = *itor++;
				size_t count = *itor++;
				glDrawArrays(mode, first, count);
			}
			break;

		case 0xf6e885d9 /* "glDrawElements" */:
			{
				GLenum mode = *itor++;
				GLsizei count = *itor++;
				glDrawElements(mode, count, GL_UNSIGNED_SHORT, &*itor);
				itor += count*sizeof(unsigned short)/sizeof(unsigned int);
			}
			break;

		case 0xd99ba82a /* "repeat" */:
			{
				int repeat = *itor++;
				int length = *itor++;
				for (int i = 0; i < repeat; i++)
					ExecuteDrawItems(itor, length, param);
				itor += length;
			}
			break;

		default:
			break;
		}
	}
}
