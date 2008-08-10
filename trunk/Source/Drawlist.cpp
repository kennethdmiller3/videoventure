#include "StdAfx.h"
#include "Interpolator.h"
#include "Drawlist.h"

enum DrawlistOp
{
	DO_glAccum, //(GLenum op, GLfloat value)
	DO_glAlphaFunc, //(GLenum func, GLclampf ref)
	DO_glArrayElement, //(GLint i)
	DO_glBegin, //(GLenum mode)
	DO_glBindTexture, //(GLenum target, GLuint texture)
	DO_glBitmap, //(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
	DO_glBlendFunc, //(GLenum sfactor, GLenum dfactor)
	DO_glCallList, //(GLuint list)
	DO_glCallLists, //(GLsizei n, GLenum type, const GLvoid *lists)
	DO_glClear, //(GLbitfield mask)
	DO_glClearAccum, //(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	DO_glClearColor, //(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
	DO_glClearDepth, //(GLclampd depth)
	DO_glClearIndex, //(GLfloat c)
	DO_glClearStencil, //(GLint s)
	DO_glClipPlane, //(GLenum plane, const GLdouble *equation)
	DO_glColor4f, //(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	DO_glColor4fv, //(const GLfloat *v)
	DO_glColorMask, //(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
	DO_glColorMaterial, //(GLenum face, GLenum mode)
	DO_glColorPointer, //(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	DO_glCopyPixels, //(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
	DO_glCopyTexImage1D, //(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
	DO_glCopyTexImage2D, //(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
	DO_glCopyTexSubImage1D, //(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
	DO_glCopyTexSubImage2D, //(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
	DO_glCullFace, //(GLenum mode)
	DO_glDeleteLists, //(GLuint list, GLsizei range)
	DO_glDeleteTextures, //(GLsizei n, const GLuint *textures)
	DO_glDepthFunc, //(GLenum func)
	DO_glDepthMask, //(GLboolean flag)
	DO_glDepthRange, //(GLclampd zNear, GLclampd zFar)
	DO_glDisable, //(GLenum cap)
	DO_glDisableClientState, //(GLenum array)
	DO_glDrawArrays, //(GLenum mode, GLint first, GLsizei count)
	DO_glDrawBuffer, //(GLenum mode)
	DO_glDrawElements, //(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
	DO_glDrawPixels, //(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
	DO_glEdgeFlag, //(GLboolean flag)
	DO_glEdgeFlagPointer, //(GLsizei stride, const GLvoid *pointer)
	DO_glEdgeFlagv, //(const GLboolean *flag)
	DO_glEnable, //(GLenum cap)
	DO_glEnableClientState, //(GLenum array)
	DO_glEnd, //(void)
	DO_glEndList, //(void)
	DO_glEvalCoord1f, //(GLfloat u)
	DO_glEvalCoord1fv, //(const GLfloat *u)
	DO_glEvalCoord2f, //(GLfloat u, GLfloat v)
	DO_glEvalCoord2fv, //(const GLfloat *u)
	DO_glEvalMesh1, //(GLenum mode, GLint i1, GLint i2)
	DO_glEvalMesh2, //(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
	DO_glEvalPoint1, //(GLint i)
	DO_glEvalPoint2, //(GLint i, GLint j)
	DO_glFeedbackBuffer, //(GLsizei size, GLenum type, GLfloat *buffer)
	DO_glFinish, //(void)
	DO_glFlush, //(void)
	DO_glFogf, //(GLenum pname, GLfloat param)
	DO_glFogfv, //(GLenum pname, const GLfloat *params)
	DO_glFrontFace, //(GLenum mode)
	DO_glFrustum, //(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
	DO_glGenLists, //(GLsizei range)
	DO_glGenTextures, //(GLsizei n, GLuint *textures)
	DO_glHint, //(GLenum target, GLenum mode)
	DO_glIndexMask, //(GLuint mask)
	DO_glIndexPointer, //(GLenum type, GLsizei stride, const GLvoid *pointer)
	DO_glIndexf, //(GLfloat c)
	DO_glIndexfv, //(const GLfloat *c)
	DO_glInitNames, //(void)
	DO_glInterleavedArrays, //(GLenum format, GLsizei stride, const GLvoid *pointer)
	DO_glLightModelf, //(GLenum pname, GLfloat param)
	DO_glLightModelfv, //(GLenum pname, const GLfloat *params)
	DO_glLightf, //(GLenum light, GLenum pname, GLfloat param)
	DO_glLightfv, //(GLenum light, GLenum pname, const GLfloat *params)
	DO_glLineStipple, //(GLint factor, GLushort pattern)
	DO_glLineWidth, //(GLfloat width)
	DO_glListBase, //(GLuint base)
	DO_glLoadIdentity, //(void)
	DO_glLoadMatrixf, //(const GLfloat *m)
	DO_glLoadName, //(GLuint name)
	DO_glLogicOp, //(GLenum opcode)
	DO_glMap1f, //(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
	DO_glMap2f, //(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
	DO_glMapGrid1f, //(GLint un, GLfloat u1, GLfloat u2)
	DO_glMapGrid2f, //(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
	DO_glMaterialf, //(GLenum face, GLenum pname, GLfloat param)
	DO_glMaterialfv, //(GLenum face, GLenum pname, const GLfloat *params)
	DO_glMatrixMode, //(GLenum mode)
	DO_glMultMatrixf, //(const GLfloat *m)
	DO_glNewList, //(GLuint list, GLenum mode)
	DO_glNormal3f, //(GLfloat nx, GLfloat ny, GLfloat nz)
	DO_glNormal3fv, //(const GLfloat *v)
	DO_glNormalPointer, //(GLenum type, GLsizei stride, const GLvoid *pointer)
	DO_glOrtho, //(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
	DO_glPassThrough, //(GLfloat token)
	DO_glPixelMapfv, //(GLenum map, GLsizei mapsize, const GLfloat *values)
	DO_glPixelStoref, //(GLenum pname, GLfloat param)
	DO_glPixelTransferf, //(GLenum pname, GLfloat param)
	DO_glPixelZoom, //(GLfloat xfactor, GLfloat yfactor)
	DO_glPointSize, //(GLfloat size)
	DO_glPolygonMode, //(GLenum face, GLenum mode)
	DO_glPolygonOffset, //(GLfloat factor, GLfloat units)
	DO_glPolygonStipple, //(const GLubyte *mask)
	DO_glPopAttrib, //(void)
	DO_glPopClientAttrib, //(void)
	DO_glPopMatrix, //(void)
	DO_glPopName, //(void)
	DO_glPrioritizeTextures, //(GLsizei n, const GLuint *textures, const GLclampf *priorities)
	DO_glPushAttrib, //(GLbitfield mask)
	DO_glPushClientAttrib, //(GLbitfield mask)
	DO_glPushMatrix, //(void)
	DO_glPushName, //(GLuint name)
	DO_glRasterPos4f, //(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
	DO_glRasterPos4fv, //(const GLfloat *v)
	DO_glReadBuffer, //(GLenum mode)
	DO_glReadPixels, //(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
	DO_glRectf, //(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
	DO_glRectfv, //(const GLfloat *v1, const GLfloat *v2)
	DO_glRotatef, //(GLfloat anDO_gle, GLfloat x, GLfloat y, GLfloat z)
	DO_glScalef, //(GLfloat x, GLfloat y, GLfloat z)
	DO_glScissor, //(GLint x, GLint y, GLsizei width, GLsizei height)
	DO_glSelectBuffer, //(GLsizei size, GLuint *buffer)
	DO_glShadeModel, //(GLenum mode)
	DO_glStencilFunc, //(GLenum func, GLint ref, GLuint mask)
	DO_glStencilMask, //(GLuint mask)
	DO_glStencilOp, //(GLenum fail, GLenum zfail, GLenum zpass)
	DO_glTexCoord4f, //(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
	DO_glTexCoord4fv, //(const GLfloat *v)
	DO_glTexCoordPointer, //(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	DO_glTexEnvf, //(GLenum target, GLenum pname, GLfloat param)
	DO_glTexEnvfv, //(GLenum target, GLenum pname, const GLfloat *params)
	DO_glTexGenf, //(GLenum coord, GLenum pname, GLfloat param)
	DO_glTexGenfv, //(GLenum coord, GLenum pname, const GLfloat *params)
	DO_glTexImage1D, //(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
	DO_glTexImage2D, //(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
	DO_glTexParameterf, //(GLenum target, GLenum pname, GLfloat param)
	DO_glTexParameterfv, //(GLenum target, GLenum pname, const GLfloat *params)
	DO_glTexSubImage1D, //(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
	DO_glTexSubImage2D, //(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
	DO_glTranslatef, //(GLfloat x, GLfloat y, GLfloat z)
	DO_glVertex4f, //(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
	DO_glVertex4fv, //(const GLfloat *v)
	DO_glVertexPointer, //(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	DO_glViewport, //(GLint x, GLint y, GLsizei width, GLsizei height)

	DO_Repeat,
	DO_Block,
	DO_Set,
	DO_Add,
	DO_Sub,
	DO_Mul,
	DO_Div,
	DO_Clear,
#ifdef DRAWLIST_LOOP
	DO_Loop,
#endif
#ifdef DRAWLIST_EMITTER
	DO_Emitter,
#endif
};

enum DrawlistDatatype
{
	DD_Literal,
	DD_Variable,
	DD_Interpolator,
	DD_Random,
};

// attribute names
static const char * sPositionNames[] = { "x", "y", "z", "w" };
static const float sPositionDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const char * sRotationNames[] = { "angle", "x", "y", "z" };
static const float sRotationDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const char * sScaleNames[] = { "x", "y", "z" };
static const float sScaleDefault[] = { 1.0f, 1.0f, 1.0f };
static const char * sColorNames[] = { "r", "g", "b", "a" };
static const float sColorDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const char * sTexCoordNames[] = { "s", "t", "r", "q" };
static const float sTexCoordDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const char * sIndexNames[] = { "c" };
static const float sIndexDefault[] = { 0.0f };
static const char * sMatrixNames[] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15" };
static const float sMatrixDefault[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

void GetTypeData(unsigned int type, int &width, const char **&names, const float *&data)
{
	switch (type)
	{
	default:
	case 0x934f4e0a /* "position" */:	names = sPositionNames; data = sPositionDefault; width = 4; break;
	case 0x21ac415f /* "rotation" */:	names = sRotationNames; data = sRotationDefault; width = 4; break;
	case 0xad0ecfd5 /* "translate" */:	names = sPositionNames, data = sPositionDefault; width = 3; break;
	case 0x82971c71 /* "scale" */:		names = sScaleNames; data = sScaleDefault; width = 3; break;
	case 0x3d7e6258 /* "color" */:		names = sColorNames; data = sColorDefault; width = 4; break;
	case 0xdd612dd3 /* "texcoord" */:	names = sTexCoordNames; data = sTexCoordDefault; width = 4; break;
	case 0x090aa9ab /* "index" */:		names = sIndexNames; data = sIndexDefault; width = 1; break;
	case 0x15c2f8ec /* "matrix" */:		names = sMatrixNames; data = sMatrixDefault; width = 16; break;
	}
}

namespace Database
{
	Typed<std::vector<unsigned int> > dynamicdrawlist(0xdf3cf9c0 /* "dynamicdrawlist" */);
	Typed<GLuint> drawlist(0xc98b019b /* "drawlist" */);
	Typed<GLuint> texture(0x3c6468f4 /* "texture" */);
	Typed<Typed<float> > variable(0x19385305 /* "variable" */);

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
				std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Open(handle);
				ProcessDrawItems(element, drawlist);
				ExecuteDrawItems(&drawlist[0], drawlist.size(), param, aId);
				Database::dynamicdrawlist.Close(handle);

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

		class VariableLoader
		{
		public:
			VariableLoader()
			{
				AddConfigure(0x19385305 /* "variable" */, Entry(this, &VariableLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Typed<float> &variables = Database::variable.Open(aId);

				unsigned int name = Hash(element->Attribute("name"));
				unsigned int type = Hash(element->Attribute("type"));
				int width;
				const char **names;
				const float *data;
				GetTypeData(type, width, names, data);
				for (int i = 0; i < width; i++)
				{
					float value = data[i];
					element->QueryFloatAttribute(names[i], &value);
					variables.Put(name+i, value);
				}

				Database::variable.Close(aId);
			}
		}
		variableloader;
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

void ProcessDrawData(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char *names[], const float data[])
{
	if (const char *name = element->Attribute("variable"))
	{
		// push a reference to a variable value
		buffer.push_back(DD_Variable);
		buffer.push_back(Hash(name));
	}
	else if (element->FirstChildElement())
	{
		// push an interpolator
		buffer.push_back(DD_Interpolator);
		buffer.push_back(0);
		int start = buffer.size();
		ProcessInterpolatorItem(element, buffer, width, names, data);
		buffer[start - 1] = buffer.size() - start;
	}
	else if (element->Attribute("rand"))
	{
		buffer.push_back(DD_Random);
		for (int i = 0; i < width; i++)
		{
			char label[64];
			sprintf(label, "%s_avg", names[i]);
			float average = data[i];
			element->QueryFloatAttribute(label, &average);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&average));
			sprintf(label, "%s_var", names[i]);
			float variance = 0.0f;
			element->QueryFloatAttribute(label, &variance);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&variance));
		}
	}
	else
	{
		// push literal data
		buffer.push_back(DD_Literal);
		for (int i = 0; i < width; i++)
		{
			float value = data[i];
			element->QueryFloatAttribute(names[i], &value);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
		}
	}
}

void ProcessFloatData(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	const char *text = element->GetText();
	size_t len = strlen(text)+1;
	char *buf = static_cast<char *>(_alloca(len));
	memcpy(buf, text, len);

	char *item = strtok(buf, " \t\n\r,;");
	while (element)
	{
		float value = float(atof(item));
		buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
		item = strtok(NULL, " \t\n\r,;");
	}
}

void ProcessVariableOperator(const TiXmlElement *element, std::vector<unsigned int> &buffer, DrawlistOp op, bool drawdata)
{
	unsigned int name = Hash(element->Attribute("name"));
	unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char **names;
	const float *data;
	GetTypeData(type, width, names, data);

	buffer.push_back(op);
	buffer.push_back(name);
	buffer.push_back(width);
	if (drawdata)
		ProcessDrawData(element, buffer, width, names, data);
}

void ProcessPrimitive(const TiXmlElement *element, std::vector<unsigned int> &buffer, GLenum mode)
{
	buffer.push_back(DO_glBegin);
	buffer.push_back(mode);
	ProcessDrawItems(element, buffer);
	buffer.push_back(DO_glEnd);
}

void ProcessArray(const TiXmlElement *element, std::vector<unsigned int> &buffer, DrawlistOp op, size_t size, size_t stride)
{
	buffer.push_back(op);
	if (size)
		buffer.push_back(size);
	buffer.push_back(stride);

	buffer.push_back(0);
	int start = buffer.size();
	ProcessFloatData(element, buffer);
	buffer[start-1] = buffer.size() - start;
}

GLenum GetPrimitiveMode(const char *mode)
{
	switch (Hash(mode))
	{
	case 0xbc9567c6 /* "points" */:			return GL_POINTS; break;
	case 0xe1e4263c /* "lines" */:			return GL_LINES; break;
	case 0xc2106ab6 /* "line_loop" */:		return GL_LINE_LOOP; break;
	case 0xc6f2fa0e /* "line_strip" */:		return GL_LINE_STRIP; break;
	case 0xd8a57342 /* "triangles" */:		return GL_TRIANGLES; break;
	case 0x668b2dd8 /* "triangle_strip" */:	return GL_TRIANGLE_STRIP; break;
	case 0xcfa6904f /* "triangle_fan" */:	return GL_TRIANGLE_FAN; break;
	case 0x5667b307 /* "quads" */:			return GL_QUADS; break;
	case 0xb47cad9b /* "quad_strip" */:		return GL_QUAD_STRIP; break;
	case 0x051cb889 /* "polygon" */:		return GL_POLYGON; break;
	default:								return 0; break;
	}
}

void ProcessDrawItem(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x974c9474 /* "pushmatrix" */:
		{
			buffer.push_back(DO_glPushMatrix);
			ProcessDrawItems(element, buffer);
			buffer.push_back(DO_glPopMatrix);
		}
		break;

	case 0x937cff81 /* "pushattrib" */:
		{
			GLuint mask = 0U;
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				GLuint bit = 0U;
				switch (Hash(attrib->Name()))
				{
				case 0xd965bbda /* "current" */:			bit = GL_CURRENT_BIT; break;
				case 0x18ae6c91 /* "point" */:				bit = GL_POINT_BIT; break;
				case 0x17db1627 /* "line" */:				bit = GL_LINE_BIT; break;
				case 0x051cb889 /* "polygon" */:			bit = GL_POLYGON_BIT; break;
				case 0x67b14997 /* "polygon_stipple" */:	bit = GL_POLYGON_STIPPLE_BIT; break;
				case 0xccde91eb /* "pixel_mode" */:			bit = GL_PIXEL_MODE_BIT; break;
				case 0x827eb1c9 /* "lighting" */:			bit = GL_LIGHTING_BIT; break;
				case 0xa1f3723f /* "fog" */:				bit = GL_FOG_BIT; break;
				case 0x65e5b825 /* "depth_buffer" */:		bit = GL_DEPTH_BUFFER_BIT; break;
				case 0x907f6213 /* "accum_buffer" */:		bit = GL_ACCUM_BUFFER_BIT; break;
				case 0x632020be /* "stencil_buffer" */:		bit = GL_STENCIL_BUFFER_BIT; break;
				case 0xe4abbac3 /* "viewport" */:			bit = GL_VIEWPORT_BIT; break;
				case 0xe1ad931b /* "transform" */:			bit = GL_TRANSFORM_BIT; break;
				case 0xaf8bb8ce /* "enable" */:				bit = GL_ENABLE_BIT; break;
				case 0x0d759bbb /* "color_buffer" */:		bit = GL_COLOR_BUFFER_BIT; break;
				case 0x4bc809b8 /* "hint" */:				bit = GL_HINT_BIT; break;
				case 0x08d22e0f /* "eval" */:				bit = GL_EVAL_BIT; break;
				case 0x0cfb5881 /* "list" */:				bit = GL_LIST_BIT; break;
				case 0x3c6468f4 /* "texture" */:			bit = GL_TEXTURE_BIT; break;
				case 0x0adbc081 /* "scissor" */:			bit = GL_SCISSOR_BIT; break;
				case 0x13254bc4 /* "all" */:				bit = GL_ALL_ATTRIB_BITS; break;
				}
				if (attrib->IntValue())
					mask |= bit;
				else
					mask &= ~bit;
			}
			buffer.push_back(DO_glPushAttrib);
			buffer.push_back(mask);
			ProcessDrawItems(element, buffer);
			buffer.push_back(DO_glPopAttrib);
		}
		break;

	case 0x052eb8b2 /* "pushclientattrib" */:
		{
			GLuint mask = 0U;
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				GLuint bit = 0U;
				switch (Hash(attrib->Name()))
				{
				case 0x959fee19 /* "pixel_store" */:	bit = GL_CLIENT_PIXEL_STORE_BIT; break;
				case 0x20a16825 /* "vertex_array" */:	bit = GL_CLIENT_VERTEX_ARRAY_BIT; break;
				case 0x13254bc4 /* "all" */:			bit = GL_CLIENT_ALL_ATTRIB_BITS; break;
				}
				if (attrib->IntValue())
					mask |= bit;
				else
					mask &= ~bit;
			}
			buffer.push_back(DO_glPushClientAttrib);
			buffer.push_back(mask);
			ProcessDrawItems(element, buffer);
			buffer.push_back(DO_glPopClientAttrib);
		}
		break;

	case 0xad0ecfd5 /* "translate" */:
		{
			buffer.push_back(DO_glTranslatef);
			ProcessDrawData(element, buffer, 3, sPositionNames, sPositionDefault);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			buffer.push_back(DO_glRotatef);
			ProcessDrawData(element, buffer, 4, sRotationNames, sRotationDefault);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			buffer.push_back(DO_glScalef);
			ProcessDrawData(element, buffer, 3, sScaleNames, sScaleDefault);
		}
		break;

	case 0x938fc4f7 /* "loadidentity" */:
		{
			buffer.push_back(DO_glLoadIdentity);
		}
		break;

	case 0x7d22a710 /* "loadmatrix" */:
		{
			buffer.push_back(DO_glLoadMatrixf);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				float m = sMatrixDefault[i];
				element->QueryFloatAttribute(name, &m);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m));
			}
		}
		break;

	case 0x3807cb92 /* "multmatrix" */:
		{
			buffer.push_back(DO_glMultMatrixf);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				float m = sMatrixDefault[i];
				element->QueryFloatAttribute(name, &m);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m));
			}
		}
		break;

	case 0x945367a7 /* "vertex" */:
		{
			buffer.push_back(DO_glVertex4fv);
			ProcessDrawData(element, buffer, 4, sPositionNames, sPositionDefault);
		}
		break;
	case 0xe68b9c52 /* "normal" */:
		{
			buffer.push_back(DO_glNormal3fv);
			ProcessDrawData(element, buffer, 3, sPositionNames, sPositionDefault);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			buffer.push_back(DO_glColor4fv);
			ProcessDrawData(element, buffer, 4, sColorNames, sColorDefault);
		}
		break;

	case 0x090aa9ab /* "index" */:
		{
			buffer.push_back(DO_glIndexf);
			ProcessDrawData(element, buffer, 1, sIndexNames, sIndexDefault);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			buffer.push_back(DO_glTexCoord4f);
			ProcessDrawData(element, buffer, 4, sTexCoordNames, sTexCoordDefault);
		}
		break;

	case 0x0135ab46 /* "edgeflag" */:
		{
			int flag;
			if (element->QueryIntAttribute("flag", &flag) == TIXML_SUCCESS)
			{
				buffer.push_back(DO_glEdgeFlag);
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
					buffer.push_back(DO_glEnable);
					buffer.push_back(GL_TEXTURE_2D);
					buffer.push_back(DO_glBindTexture);
					buffer.push_back(GL_TEXTURE_2D);
					buffer.push_back(texture);
				}
			}
		}
		break;

	case 0xbc9567c6 /* "points" */:
		{
			ProcessPrimitive(element, buffer, GL_POINTS);
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			ProcessPrimitive(element, buffer, GL_LINES);
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			ProcessPrimitive(element, buffer, GL_LINE_LOOP);
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			ProcessPrimitive(element, buffer, GL_LINE_STRIP);
		}
		break;

	case 0xd8a57342 /* "triangles" */:
		{
			ProcessPrimitive(element, buffer, GL_TRIANGLES);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			ProcessPrimitive(element, buffer, GL_TRIANGLE_STRIP);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			ProcessPrimitive(element, buffer, GL_TRIANGLE_FAN);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			ProcessPrimitive(element, buffer, GL_QUADS);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			ProcessPrimitive(element, buffer, GL_QUAD_STRIP);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			ProcessPrimitive(element, buffer, GL_POLYGON);
		}
		break;

	case 0xe3e74f7e /* "blendfunc" */:
		{
			const char *src = element->Attribute("src");
			GLenum srcfactor;
			switch(Hash(src))
			{
			case 0x8b6fe763 /* "zero" */:					srcfactor = GL_ZERO; break;
			default:
			case 0xba2719ef /* "one" */:					srcfactor = GL_ONE; break;
			case 0x7fe1449a /* "src_alpha" */:				srcfactor = GL_SRC_ALPHA; break;
			case 0x1d491ada /* "one_minus_src_alpha" */:	srcfactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case 0x201e0c0f /* "dst_alpha" */:				srcfactor = GL_DST_ALPHA; break;
			case 0x1ad7d24f /* "one_minus_dst_alpha" */:	srcfactor = GL_ONE_MINUS_DST_ALPHA; break;
			case 0xc913d33c /* "dst_color" */:				srcfactor = GL_DST_COLOR; break;
			case 0xc3cc067c /* "one_minus_dst_color" */:	srcfactor = GL_ONE_MINUS_DST_COLOR; break;
			case 0xe03d4f7e /* "src_alpha_saturate" */:		srcfactor = GL_SRC_ALPHA_SATURATE; break;
			}

			const char *dst = element->Attribute("dst");
			GLenum dstfactor;
			switch(Hash(dst))
			{
			default:
			case 0x8b6fe763 /* "zero" */:					dstfactor = GL_ZERO; break;
			case 0xba2719ef /* "one" */:					dstfactor = GL_ONE; break;
			case 0x4dba79b5 /* "src_color" */:				dstfactor = GL_SRC_COLOR; break;
			case 0xd1d59af5 /* "one_minus_src_color" */:	dstfactor = GL_ONE_MINUS_SRC_COLOR; break;
			case 0x7fe1449a /* "src_alpha" */:				dstfactor = GL_SRC_ALPHA; break;
			case 0x1d491ada /* "one_minus_src_alpha" */:	dstfactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case 0x201e0c0f /* "dst_alpha" */:				dstfactor = GL_DST_ALPHA; break;
			case 0x1ad7d24f /* "one_minus_dst_alpha" */:	dstfactor = GL_ONE_MINUS_DST_ALPHA; break;
			}

			buffer.push_back(DO_glBlendFunc);
			buffer.push_back(srcfactor);
			buffer.push_back(dstfactor);
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
					buffer.push_back(DO_glCallList);
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

			// register the draw list
			Database::drawlist.Put(handle, handle);

			// get (optional) parameter value
			float param = 0.0f;
			element->QueryFloatAttribute("param", &param);

			// process draw items
			std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Open(handle);
			ProcessDrawItems(element, drawlist);
			ExecuteDrawItems(&drawlist[0], drawlist.size(), param, 0);
			Database::dynamicdrawlist.Close(handle);

			// finish the draw list
			glEndList();

			// use the anonymous drawlist
			buffer.push_back(DO_glCallList);
			buffer.push_back(handle);
		}
		break;

	case 0x2610a4a3 /* "clientstate" */:
		{
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				unsigned int action = attrib->IntValue() ? DO_glEnableClientState : DO_glDisableClientState;
				GLenum clientstate;
				switch (Hash(attrib->Name()))
				{
				case 0x945367a7 /* "vertex" */:		clientstate = GL_VERTEX_ARRAY; break;
				case 0xe68b9c52 /* "normal" */:		clientstate = GL_NORMAL_ARRAY; break;
				case 0x3d7e6258 /* "color" */:		clientstate = GL_COLOR_ARRAY; break;
				case 0x090aa9ab /* "index" */:		clientstate = GL_INDEX_ARRAY; break;
				case 0xdd612dd3 /* "texcoord" */:	clientstate = GL_TEXTURE_COORD_ARRAY; break;
				case 0x0135ab46 /* "edgeflag" */:	clientstate = GL_EDGE_FLAG_ARRAY; break;
				default:							clientstate = 0; break;
				}
				if (clientstate)
				{
					buffer.push_back(action);
					buffer.push_back(clientstate);
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

			ProcessArray(element, buffer, DO_glVertexPointer, size, stride);
		}
		break;

	case 0x81491d33 /* "normalarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ProcessArray(element, buffer, DO_glNormalPointer, 0, stride);
		}
		break;

	case 0xcce5b995 /* "colorarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ProcessArray(element, buffer, DO_glColorPointer, size, stride);
		}
		break;

	case 0x664ead80 /* "indexarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ProcessArray(element, buffer, DO_glIndexPointer, 0, stride);
		}
		break;

	case 0x91aa3148 /* "texcoordarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ProcessArray(element, buffer, DO_glTexCoordPointer, size, stride);
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

			buffer.push_back(DO_glEdgeFlagPointer);
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
				buffer.push_back(DO_glArrayElement);
				buffer.push_back(index);
			}
		}
		break;

	case 0xf4de4a21 /* "drawarrays" */:
		{
			GLenum mode(GetPrimitiveMode(element->Attribute("mode")));

			int first = 0, count = 0;
			element->QueryIntAttribute("first", &first);
			element->QueryIntAttribute("count", &count);
			buffer.push_back(DO_glDrawArrays);
			buffer.push_back(mode);
			buffer.push_back(first);
			buffer.push_back(count);
		}
		break;

	case 0x757eeee2 /* "drawelements" */:
		{
			GLenum mode(GetPrimitiveMode(element->Attribute("mode")));

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

			buffer.push_back(DO_glDrawElements);
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

			buffer.push_back(DO_Repeat);
			buffer.push_back(count);

			buffer.push_back(0);
			int start = buffer.size();
			ProcessDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;

	case 0xeb0cbd62 /* "block" */:
		{
			float start = 0.0f;
			element->QueryFloatAttribute("start", &start);
			float length = FLT_MAX;
			element->QueryFloatAttribute("length", &length);
			float scale = 1.0f;
			element->QueryFloatAttribute("scale", &scale);

			buffer.push_back(DO_Block);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&start));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&length));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&scale));

			buffer.push_back(0);
			int size = buffer.size();
			ProcessDrawItems(element, buffer);
			buffer[size-1] = buffer.size() - size;
		}
		break;

	case 0xc6270703 /* "set" */:
		{
			ProcessVariableOperator(element, buffer, DO_Set, true);
		}
		break;

	case 0x3b391274 /* "add" */:
		{
			ProcessVariableOperator(element, buffer, DO_Add, true);
		}
		break;

	case 0xdc4e3915 /* "sub" */:
		{
			ProcessVariableOperator(element, buffer, DO_Sub, true);
		}
		break;

	case 0xeb84ed81 /* "mul" */:
		{
			ProcessVariableOperator(element, buffer, DO_Mul, true);
		}
		break;

	case 0xe562ab48 /* "div" */:
		{
			ProcessVariableOperator(element, buffer, DO_Div, true);
		}
		break;

	case 0x5c6e1222 /* "clear" */:
		{
			ProcessVariableOperator(element, buffer, DO_Clear, true);
		}
		break;

#ifdef DRAWLIST_LOOP
	case 0xddef486b /* "loop" */:
		{
			unsigned int name = Hash(element->Attribute("name"));
			float from = 0.0f;
			element->QueryFloatAttribute("from", &from);
			float to = 0.0f;
			element->QueryFloatAttribute("to", &to);
			float by = 0.0f;
			element->QueryFloatAttribute("by", &by);

			buffer.push_back(DO_Loop);
			buffer.push_back(name);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&from));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&to));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&by));

			buffer.push_back(0);
			int start = buffer.size();
			ProcessDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;
#endif

#ifdef DRAWLIST_EMITTER
	case 0x576b09cd /* "emitter" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);
			float period = 1.0f;
			element->QueryFloatAttribute("period", &period);
			float x = 0.0f, y = 0.0f, a = 0.0f;
			element->QueryFloatAttribute("x", &x);
			element->QueryFloatAttribute("y", &y);
			element->QueryFloatAttribute("angle", &a);

			buffer.push_back(DO_Emitter);
			buffer.push_back(count);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&period));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&x));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&y));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&a));

			buffer.push_back(0);
			int start = buffer.size();
			ProcessDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;
#endif

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

#pragma optimize( "t", on )
size_t ExecuteDrawData(const unsigned int buffer[], size_t count, int width, float data[], float param, int id)
{
	const unsigned int * __restrict itor = buffer;
	switch(*itor++)
	{
	case DD_Literal:
		{
			// get literal value
			for (int i = 0; i < width; i++)
				reinterpret_cast<unsigned int * __restrict>(data)[i] = *itor++;
		}
		break;

	case DD_Variable:
		{
			// TO DO: get the global value
			unsigned int name = *itor++;
			const Database::Typed<float> &variables = Database::variable.Get(id);
			for (int i = 0; i < width; i++)
				data[i] = variables.Get(name+i);
		}
		break;

	case DD_Interpolator:
		{
			unsigned int size = *itor++;

			// get interpolator value
			const int count = *itor;
			const float * __restrict keys = reinterpret_cast<const float * __restrict>(itor+1);
			int dummy = 0;
			if (!ApplyInterpolator(data, width, count, keys, param, dummy))
				memset(data, 0, width*sizeof(float));

			itor += size;
		}
		break;

	case DD_Random:
		{
			// generate random value
			for (int i = 0; i < width; i++)
			{
				float average = *reinterpret_cast<const float * __restrict>(itor++);
				float variance = *reinterpret_cast<const float * __restrict>(itor++);
				data[i] = RandValue(average, variance);
			}
		}
		break;

	default:
		DebugPrint("Unrecognized drawlist datatype 0x%08x at index %d", *(itor-1), itor-buffer);
		break;
	}

	return itor - buffer;
}
#pragma optimize("", on)


#ifdef DRAWLIST_EMITTER
float Determinant4f(const float m[16])
{
	return
		m[12]*m[9]*m[6]*m[3]-
		m[8]*m[13]*m[6]*m[3]-
		m[12]*m[5]*m[10]*m[3]+
		m[4]*m[13]*m[10]*m[3]+
		m[8]*m[5]*m[14]*m[3]-
		m[4]*m[9]*m[14]*m[3]-
		m[12]*m[9]*m[2]*m[7]+
		m[8]*m[13]*m[2]*m[7]+
		m[12]*m[1]*m[10]*m[7]-
		m[0]*m[13]*m[10]*m[7]-
		m[8]*m[1]*m[14]*m[7]+
		m[0]*m[9]*m[14]*m[7]+
		m[12]*m[5]*m[2]*m[11]-
		m[4]*m[13]*m[2]*m[11]-
		m[12]*m[1]*m[6]*m[11]+
		m[0]*m[13]*m[6]*m[11]+
		m[4]*m[1]*m[14]*m[11]-
		m[0]*m[5]*m[14]*m[11]-
		m[8]*m[5]*m[2]*m[15]+
		m[4]*m[9]*m[2]*m[15]+
		m[8]*m[1]*m[6]*m[15]-
		m[0]*m[9]*m[6]*m[15]-
		m[4]*m[1]*m[10]*m[15]+
		m[0]*m[5]*m[10]*m[15];
}

bool InvertMatrix4f(float i[16], const float m[16])
{
	float x=Determinant4f(m);
	if (x==0) return false;
	x = 1.0f / x;
	i[ 0]= (-m[13] * m[10] * m[ 7] + m[ 9] * m[14] * m[ 7] + m[13] * m[ 6] * m[11] - m[ 5] * m[14] * m[11] - m[ 9] * m[ 6] * m[15] + m[ 5] * m[10] * m[15]) * x;
	i[ 4]= ( m[12] * m[10] * m[ 7] - m[ 8] * m[14] * m[ 7] - m[12] * m[ 6] * m[11] + m[ 4] * m[14] * m[11] + m[ 8] * m[ 6] * m[15] - m[ 4] * m[10] * m[15]) * x;
	i[ 8]= (-m[12] * m[ 9] * m[ 7] + m[ 8] * m[13] * m[ 7] + m[12] * m[ 5] * m[11] - m[ 4] * m[13] * m[11] - m[ 8] * m[ 5] * m[15] + m[ 4] * m[ 9] * m[15]) * x;
	i[12]= ( m[12] * m[ 9] * m[ 6] - m[ 8] * m[13] * m[ 6] - m[12] * m[ 5] * m[10] + m[ 4] * m[13] * m[10] + m[ 8] * m[ 5] * m[14] - m[ 4] * m[ 9] * m[14]) * x;
	i[ 1]= ( m[13] * m[10] * m[ 3] - m[ 9] * m[14] * m[ 3] - m[13] * m[ 2] * m[11] + m[ 1] * m[14] * m[11] + m[ 9] * m[ 2] * m[15] - m[ 1] * m[10] * m[15]) * x;
	i[ 5]= (-m[12] * m[10] * m[ 3] + m[ 8] * m[14] * m[ 3] + m[12] * m[ 2] * m[11] - m[ 0] * m[14] * m[11] - m[ 8] * m[ 2] * m[15] + m[ 0] * m[10] * m[15]) * x;
	i[ 9]= ( m[12] * m[ 9] * m[ 3] - m[ 8] * m[13] * m[ 3] - m[12] * m[ 1] * m[11] + m[ 0] * m[13] * m[11] + m[ 8] * m[ 1] * m[15] - m[ 0] * m[ 9] * m[15]) * x;
	i[13]= (-m[12] * m[ 9] * m[ 2] + m[ 8] * m[13] * m[ 2] + m[12] * m[ 1] * m[10] - m[ 0] * m[13] * m[10] - m[ 8] * m[ 1] * m[14] + m[ 0] * m[ 9] * m[14]) * x;
	i[ 2]= (-m[13] * m[ 6] * m[ 3] + m[ 5] * m[14] * m[ 3] + m[13] * m[ 2] * m[ 7] - m[ 1] * m[14] * m[ 7] - m[ 5] * m[ 2] * m[15] + m[ 1] * m[ 6] * m[15]) * x;
	i[ 6]= ( m[12] * m[ 6] * m[ 3] - m[ 4] * m[14] * m[ 3] - m[12] * m[ 2] * m[ 7] + m[ 0] * m[14] * m[ 7] + m[ 4] * m[ 2] * m[15] - m[ 0] * m[ 6] * m[15]) * x;
	i[10]= (-m[12] * m[ 5] * m[ 3] + m[ 4] * m[13] * m[ 3] + m[12] * m[ 1] * m[ 7] - m[ 0] * m[13] * m[ 7] - m[ 4] * m[ 1] * m[15] + m[ 0] * m[ 5] * m[15]) * x;
	i[14]= ( m[12] * m[ 5] * m[ 2] - m[ 4] * m[13] * m[ 2] - m[12] * m[ 1] * m[ 6] + m[ 0] * m[13] * m[ 6] + m[ 4] * m[ 1] * m[14] - m[ 0] * m[ 5] * m[14]) * x;
	i[ 3]= ( m[ 9] * m[ 6] * m[ 3] - m[ 5] * m[10] * m[ 3] - m[ 9] * m[ 2] * m[ 7] + m[ 1] * m[10] * m[ 7] + m[ 5] * m[ 2] * m[11] - m[ 1] * m[ 6] * m[11]) * x;
	i[ 7]= (-m[ 8] * m[ 6] * m[ 3] + m[ 4] * m[10] * m[ 3] + m[ 8] * m[ 2] * m[ 7] - m[ 0] * m[10] * m[ 7] - m[ 4] * m[ 2] * m[11] + m[ 0] * m[ 6] * m[11]) * x;
	i[11]= ( m[ 8] * m[ 5] * m[ 3] - m[ 4] * m[ 9] * m[ 3] - m[ 8] * m[ 1] * m[ 7] + m[ 0] * m[ 9] * m[ 7] + m[ 4] * m[ 1] * m[11] - m[ 0] * m[ 5] * m[11]) * x;
	i[15]= (-m[ 8] * m[ 5] * m[ 2] + m[ 4] * m[ 9] * m[ 2] + m[ 8] * m[ 1] * m[ 6] - m[ 0] * m[ 9] * m[ 6] - m[ 4] * m[ 1] * m[10] + m[ 0] * m[ 5] * m[10]) * x;
	return true;
}

void MultiplyMatrix4f(float m[16], float a[16], float b[16])
{
	m[ 0] = a[ 0] * b[ 0] + a[ 1] * b[ 4] + a[ 2] * b[ 8] + a[ 3] * b[12];
	m[ 1] = a[ 0] * b[ 1] + a[ 1] * b[ 5] + a[ 2] * b[ 9] + a[ 3] * b[13];
	m[ 2] = a[ 0] * b[ 2] + a[ 1] * b[ 6] + a[ 2] * b[10] + a[ 3] * b[14];
	m[ 3] = a[ 0] * b[ 3] + a[ 1] * b[ 7] + a[ 2] * b[11] + a[ 3] * b[15];
	m[ 4] = a[ 4] * b[ 0] + a[ 5] * b[ 4] + a[ 6] * b[ 8] + a[ 7] * b[12];
	m[ 5] = a[ 4] * b[ 1] + a[ 5] * b[ 5] + a[ 6] * b[ 9] + a[ 7] * b[13];
	m[ 6] = a[ 4] * b[ 2] + a[ 5] * b[ 6] + a[ 6] * b[10] + a[ 7] * b[14];
	m[ 7] = a[ 4] * b[ 3] + a[ 5] * b[ 7] + a[ 6] * b[11] + a[ 7] * b[15];
	m[ 8] = a[ 8] * b[ 0] + a[ 9] * b[ 4] + a[10] * b[ 8] + a[11] * b[12];
	m[ 9] = a[ 8] * b[ 1] + a[ 9] * b[ 5] + a[10] * b[ 9] + a[11] * b[13];
	m[10] = a[ 8] * b[ 2] + a[ 9] * b[ 6] + a[10] * b[10] + a[11] * b[14];
	m[11] = a[ 8] * b[ 3] + a[ 9] * b[ 7] + a[10] * b[11] + a[11] * b[15];
	m[12] = a[12] * b[ 0] + a[13] * b[ 4] + a[14] * b[ 8] + a[15] * b[12];
	m[13] = a[12] * b[ 1] + a[13] * b[ 5] + a[14] * b[ 9] + a[15] * b[13];
	m[14] = a[12] * b[ 2] + a[13] * b[ 6] + a[14] * b[10] + a[15] * b[14];
	m[15] = a[12] * b[ 3] + a[13] * b[ 7] + a[14] * b[11] + a[15] * b[15];
}
#endif

typedef void (* VariableOperator)(Database::Typed<float> &, unsigned int, float);

size_t ExecuteVariableOperator(const unsigned int buffer[], size_t count, float param, unsigned int id, VariableOperator op)
{
	const unsigned int * __restrict itor = buffer;
	unsigned int name = *itor++;
	int width = *itor++;
	float *data = static_cast<float *>(_alloca(width*sizeof(float)));
	itor += ExecuteDrawData(itor, buffer + count - itor, width, data, param, id);
	Database::Typed<float> &variables = Database::variable.Open(id);
	for (int i = 0; i < width; i++)
		op(variables, name+i, data[i]);
	Database::variable.Close(id);
	return itor - buffer;
}

void VariableOperatorSet(Database::Typed<float> &variables, unsigned int name, float data)
{
	variables.Put(name, data);
}

void VariableOperatorAdd(Database::Typed<float> &variables, unsigned int name, float data)
{
	float &v = variables.Open(name);
	v += data;
	variables.Close(name);
}

void VariableOperatorSub(Database::Typed<float> &variables, unsigned int name, float data)
{
	float &v = variables.Open(name);
	v -= data;
	variables.Close(name);
}

void VariableOperatorMul(Database::Typed<float> &variables, unsigned int name, float data)
{
	float &v = variables.Open(name);
	v *= data;
	variables.Close(name);
}

void VariableOperatorDiv(Database::Typed<float> &variables, unsigned int name, float data)
{
	float &v = variables.Open(name);
	v /= data;
	variables.Close(name);
}


#pragma optimize( "t", on )
void ExecuteDrawItems(const unsigned int buffer[], size_t count, float param, unsigned int id)
{
	GLfloat data[4];

	const unsigned int * __restrict itor = buffer;
	while (itor < buffer + count)
	{
		switch (*itor++)
		{
		case DO_glArrayElement:
			glArrayElement(*itor++);
			break;

		case DO_glBegin:
			glBegin(GLenum(*itor++));
			break;

		case DO_glBindTexture:
			glBindTexture(itor[0], itor[1]);
			itor += 2;
			break;

		case DO_glBlendFunc:
			glBlendFunc(itor[0], itor[1]);
			itor += 2;
			break;

		case DO_glCallList:
			glCallList(GLuint(*itor++));
			break;

		case DO_glColor4fv:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param, id);
			glColor4fv(data);
			break;

		case DO_glColorPointer:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glColorPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case DO_glDisable:
			glDisable(*itor++);
			break;

		case DO_glDisableClientState:
			glDisableClientState(GLenum(*itor++));
			break;

		case DO_glDrawArrays:
			{
				GLenum mode = *itor++;
				GLint first = *itor++;
				size_t count = *itor++;
				glDrawArrays(mode, first, count);
			}
			break;

		case DO_glDrawElements:
			{
				GLenum mode = *itor++;
				GLsizei count = *itor++;
				glDrawElements(mode, count, GL_UNSIGNED_SHORT, &*itor);
				itor += count*sizeof(unsigned short)/sizeof(unsigned int);
			}
			break;

		case DO_glEdgeFlag:
			glEdgeFlag(*itor++ != 0);
			break;

		case DO_glEdgeFlagPointer:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glEdgeFlagPointer(stride, &*itor);
				itor += count*sizeof(bool)/sizeof(unsigned int);
			}
			break;

		case DO_glEnable:
			glEnable(*itor++);
			break;

		case DO_glEnableClientState:
			glEnableClientState(GLenum(*itor++));
			break;

		case DO_glEnd:
			glEnd();
			break;

		case DO_glIndexf:
			itor += ExecuteDrawData(itor, buffer + count - itor, 1, data, param, id);
			glIndexf(data[0]);
			break;

		case DO_glIndexPointer:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glIndexPointer(GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case DO_glLoadIdentity:
			glLoadIdentity();
			break;

		case DO_glLoadMatrixf:
			glLoadMatrixf(reinterpret_cast<const GLfloat *>(&*itor));
			itor+=16;
			break;

		case DO_glMultMatrixf:
			glMultMatrixf(reinterpret_cast<const GLfloat *>(&*itor));
			itor+=16;
			break;

		case DO_glNormal3fv:
			itor += ExecuteDrawData(itor, buffer + count - itor, 3, data, param, id);
			glNormal3fv(data);
			break;

		case DO_glNormalPointer:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glNormalPointer(GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case DO_glPopAttrib:
			glPopAttrib();
			break;

		case DO_glPopClientAttrib:
			glPopClientAttrib();
			break;

		case DO_glPopMatrix:
			glPopMatrix();
			break;

		case DO_glPushAttrib:
			glPushAttrib(GLbitfield(*itor++));
			break;

		case DO_glPushClientAttrib:
			glPushClientAttrib(GLbitfield(*itor++));
			break;

		case DO_glPushMatrix:
			glPushMatrix();
			break;

		case DO_glRotatef:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param, id);
			glRotatef(data[0], data[1], data[2], data[3]);
			break;

		case DO_glScalef:
			itor += ExecuteDrawData(itor, buffer + count - itor, 3, data, param, id);
			glScalef(data[0], data[1], data[2]);
			break;

		case DO_glTexCoord4fv:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param, id);
			glTexCoord4fv(data);
			break;

		case DO_glTexCoordPointer:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glTexCoordPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case DO_glTranslatef:
			itor += ExecuteDrawData(itor, buffer + count - itor, 3, data, param, id);
			glTranslatef(data[0], data[1], data[2]);
			break;

		case DO_glVertex4fv:
			itor += ExecuteDrawData(itor, buffer + count - itor, 4, data, param, id);
			glVertex4fv(data);
			break;

		case DO_glVertexPointer:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glVertexPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case DO_Repeat:
			{
				int repeat = *itor++;
				size_t size = *itor++;
				for (int i = 0; i < repeat; i++)
					ExecuteDrawItems(itor, size, param, id);
				itor += size;
			}
			break;

		case DO_Block:
			{
				float start = *reinterpret_cast<const float *>(itor++);
				float length = *reinterpret_cast<const float *>(itor++);
				float scale = *reinterpret_cast<const float *>(itor++);
				unsigned int size = *itor++;
				if (param >= start && param <= start + length)
					ExecuteDrawItems(itor, size, (param - start) * scale, id);
				itor += size;
			}
			break;

		case DO_Set:
			itor += ExecuteVariableOperator(itor, buffer + count - itor, param, id, VariableOperatorSet);
			break;

		case DO_Add:
			itor += ExecuteVariableOperator(itor, buffer + count - itor, param, id, VariableOperatorAdd);
			break;

		case DO_Sub:
			itor += ExecuteVariableOperator(itor, buffer + count - itor, param, id, VariableOperatorSub);
			break;

		case DO_Mul:
			itor += ExecuteVariableOperator(itor, buffer + count - itor, param, id, VariableOperatorMul);
			break;

		case DO_Div:
			itor += ExecuteVariableOperator(itor, buffer + count - itor, param, id, VariableOperatorDiv);
			break;

		case DO_Clear:
			{
				unsigned int name = *itor++;
				int width = *itor++;
				Database::Typed<float> &variables = Database::variable.Open(id);
				for (int i = 0; i < width; i++)
					variables.Delete(name+i);
				Database::variable.Close(id);
			}
			break;

#ifdef DRAWLIST_LOOP
		case DO_Loop:
			{
				unsigned int name = *itor++;
				float from = *reinterpret_cast<const float *>(itor++);
				float to   = *reinterpret_cast<const float *>(itor++);
				float by   = *reinterpret_cast<const float *>(itor++);
				size_t size = *itor++;

				Database::Typed<float> &variables = Database::variable.Open(id);
				if (by > 0)
				{
					for (float value = from; value <= to; value += by)
					{
						variables.Put(name, value);
						ExecuteDrawItems(itor, size, param, id);
					}
				}
				else
				{
					for (float value = from; value >= to; value += by)
					{
						variables.Put(name, value);
						ExecuteDrawItems(itor, size, param, id);
					}
				}
				variables.Delete(name);
				Database::variable.Close(id);

				itor += size;
			}
			break;
#endif

#ifdef DRAWLIST_EMITTER
		case DO_Emitter:
			{
				unsigned int name = Hash(&itor, sizeof(itor));
				int repeat = *itor++;
				float period = *reinterpret_cast<const float *>(itor++);
				float offsetx = *reinterpret_cast<const float *>(itor++);
				float offsety = *reinterpret_cast<const float *>(itor++);
				float offseta = *reinterpret_cast<const float *>(itor++);
				Matrix2 offset(offseta, Vector2(offsetx, offsety));
				size_t size = *itor++;

				// get the curent model matrix
				float m1[16];
				glGetFloatv(GL_MODELVIEW_MATRIX, m1);

				// return to parent model space
				glPopMatrix();

				// get the parent model matrix
				float m2[16];
				glGetFloatv(GL_MODELVIEW_MATRIX, m2);

				// compute local matrix
				float mi[16];
				InvertMatrix4f(mi, m2);
				float m[16];
				MultiplyMatrix4f(m, m1, mi);

				// get local variables
				Database::Typed<float> &variables = Database::variable.Open(id);

				// open state
				int state = -1;
				unsigned int lastid = Hash(&state, sizeof(state), name);

				// get previous state
				float x0 = variables.Get(lastid+0);
				float y0 = variables.Get(lastid+1);
				float a0 = variables.Get(lastid+2);
				float t0 = variables.Get(lastid+3);

				// get current state
				float x1 = m[12];
				float y1 = m[13];
				float a1 = atan2f(m[1], m[0]);
				float t1 = param / period;

				// save state
				variables.Put(lastid+0, x1);
				variables.Put(lastid+1, y1);
				variables.Put(lastid+2, a1);
				variables.Put(lastid+3, t1);

				// emit particles
				for (float t = ceilf(t0); t < t1; t += 1.0f)
				{
					// interpolate emitter position
					float r = (t - t0) / (t1 - t0);
					Matrix2 transform(offset * Matrix2(Lerp(a0, a1, r), Vector2(Lerp(x0, x1, r), Lerp(y0, y1, r))));

					// set particle state
					int i0 = xs_FloorToInt(t) % repeat;
					unsigned int subid = Hash(&i0, sizeof(i0), name);
					variables.Put(subid+0, transform.p.x);
					variables.Put(subid+1, transform.p.y);
					variables.Put(subid+2, transform.Angle());
					variables.Put(subid+3, t);
				}

				// draw particles
				for (int i = 0; i < repeat; i++)
				{
					unsigned int subid = Hash(&i, sizeof(i), name);
					float t = param - variables.Get(subid+3) * period;
					if (t >= 0.0f)
					{
						glPushMatrix();
						glTranslatef(variables.Get(subid+0), variables.Get(subid+1), 0.0f);
						glRotatef(variables.Get(subid+2)*180.0f/float(M_PI), 0, 0, 1);
						ExecuteDrawItems(itor, size, t, id);
						glPopMatrix();
					}
				}

				// done with local variables
				Database::variable.Close(id);

				// restore model matrix
				glPushMatrix();
				glLoadMatrixf(m1);

				// advance data pointer
				itor += size;
			}
			break;
#endif

		default:
			DebugPrint("Unrecognized drawlist operation 0x%08x at index %d", *(itor-1), itor-buffer);
			break;
		}
	}
}
#pragma optimize( "", on )

void RebuildDrawlists(void)
{
	for (Database::Typed<unsigned int>::Iterator itor(&Database::drawlist); itor.IsValid(); ++itor)
	{
		// recreate the draw list
		GLuint handle = itor.GetValue();
		glNewList(handle, GL_COMPILE);

		// TO DO: recover parameter value
		float param = 0.0f;

		// process draw items
		// TO DO: recover id value
		const std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Get(handle);
		if (drawlist.size() > 0)
		{
			ExecuteDrawItems(&drawlist[0], drawlist.size(), param, 0);
		}

		// finish the draw list
		glEndList();
	}
}
