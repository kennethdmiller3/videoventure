#include "StdAfx.h"
#include "Drawlist.h"
#include "Texture.h"
#include "Interpolator.h"
#include "Noise.h"

#include "Expression.h"

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
//	DO_glColor4f, //(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
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
	DO_glTexCoord2f, //(GLfloat s, GLfloat t)
	DO_glTexCoord2fv, //(const GLfloat *v)
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
	DO_glVertex3f, //(GLfloat x, GLfloat y, GLfloat z)
	DO_glVertex3fv, //(const GLfloat *v)
	DO_glVertexPointer, //(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	DO_glViewport, //(GLint x, GLint y, GLsizei width, GLsizei height)

	DO_Repeat,
	DO_Block,
	DO_Set,
	DO_Add,
	DO_Sub,
	DO_Mul,
	DO_Div,
	DO_Min,
	DO_Max,
	DO_Swizzle,
	DO_Clear,
#ifdef DRAWLIST_LOOP
	DO_Loop,
#endif
#ifdef DRAWLIST_EMITTER
	DO_Emitter,
#endif
};

// attribute names
static const char * const sScalarNames[] = { "value" };
static const float sScalarDefault[] = { 0.0f };
static const int sScalarWidth = 1;
static const char * const sPositionNames[] = { "x", "y", "z" };
static const float sPositionDefault[] = { 0.0f, 0.0f, 0.0f };
static const int sPositionWidth = 3;
static const char * const sRotationNames[] = { "angle" };
static const float sRotationDefault[] = { 0.0f };
static const int sRotationWidth = 1;
static const char * const sScaleNames[] = { "x", "y", "z" };
static const float sScaleDefault[] = { 1.0f, 1.0f, 1.0f };
static const int sScaleWidth = 3;
static const char * const sColorNames[] = { "r", "g", "b", "a" };
static const float sColorDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sColorWidth = 4;
static const char * const sTexCoordNames[] = { "s", "t" };
static const float sTexCoordDefault[] = { 0.0f, 0.0f };
static const int sTexCoordWidth = 2;
static const char * const sIndexNames[] = { "c" };
static const float sIndexDefault[] = { 0.0f };
static const int sIndexWidth = 1;
static const char * const sMatrixNames[] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15" };
static const float sMatrixDefault[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static const int sMatrixWidth = 16;

void GetTypeData(unsigned int type, int &width, const char * const *&names, const float *&data)
{
	switch (type)
	{
	default:
	case 0x934f4e0a /* "position" */:	names = sPositionNames; data = sPositionDefault; width = sPositionWidth; break;
	case 0x21ac415f /* "rotation" */:	names = sRotationNames; data = sRotationDefault; width = sRotationWidth; break;
	case 0xad0ecfd5 /* "translate" */:	names = sPositionNames, data = sPositionDefault; width = sPositionWidth; break;
	case 0x82971c71 /* "scale" */:		names = sScaleNames; data = sScaleDefault; width = sScaleWidth; break;
	case 0x3d7e6258 /* "color" */:		names = sColorNames; data = sColorDefault; width = sColorWidth; break;
	case 0xdd612dd3 /* "texcoord" */:	names = sTexCoordNames; data = sTexCoordDefault; width = sTexCoordWidth; break;
	case 0x090aa9ab /* "index" */:		names = sIndexNames; data = sIndexDefault; width = sIndexWidth; break;
	case 0x15c2f8ec /* "matrix" */:		names = sMatrixNames; data = sMatrixDefault; width = sMatrixWidth; break;
	}
}

namespace Database
{
	Typed<std::vector<unsigned int> > dynamicdrawlist(0xdf3cf9c0 /* "dynamicdrawlist" */);
	Typed<GLuint> drawlist(0xc98b019b /* "drawlist" */);
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
				ConfigureDrawItems(element, buffer);
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
				ConfigureDrawItems(element, drawlist);
				ExecuteDrawItems(&drawlist[0], drawlist.size(), param, aId);
				Database::dynamicdrawlist.Close(handle);

				// finish the draw list
				glEndList();
			}
		}
		drawlistloader;

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
				const char * const *names;
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

template<typename T> void ConfigureExpression(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[]);

// draw item context
// (extends expression context)
struct DrawItemContext : public Expression::Context
{
	float mParam;
	unsigned int mId;
};

namespace Expression
{
	// component counts
	template <typename T> struct ComponentCount { };
	template <> struct ComponentCount<float> { enum { VALUE = 0 }; };
	template <> struct ComponentCount<Vector2> { enum { VALUE = 2 }; };
	template <> struct ComponentCount<Vector3> { enum { VALUE = 3 }; };
	template <> struct ComponentCount<Vector4> { enum { VALUE = 4 }; };
	template <> struct ComponentCount<Color4> { enum { VALUE = 4 }; };

	// various constructors
	template<typename T> const T Construct(Context &aContext);
	template<> const float Construct<float>(Context &aContext)
	{
		return Evaluate<float>(aContext);
	}
	template<> const Vector2 Construct<Vector2>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		return Vector2(arg1, arg2);
	}
	template<> const Vector3 Construct<Vector3>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		return Vector3(arg1, arg2, arg3);
	}
	template<> const Color4 Construct<Color4>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return Color4(arg1, arg2, arg3, arg4);
	}
	template<> const Vector4 Construct<Vector4>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return Vector4(arg1, arg2, arg3, arg4);
	}

	// extend a scalar
	template<typename T, typename A> const T Extend(Context &aContext);
	template<> const float Extend<float, float>(Context &aContext)
	{
		return Evaluate<float>(aContext);
	}
	template<> const Vector2 Extend<Vector2, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector2(arg, arg);
	}
	template<> const Vector3 Extend<Vector3, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector3(arg, arg, arg);
	}
	template<> const Vector4 Extend<Vector4, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector4(arg, arg, arg, arg);
	}
	template<> const Color4 Extend<Color4, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Color4(arg, arg, arg, arg);
	}

	// aritmetic operators
	template <typename T> T Add(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 + arg2;
	}
	template <typename T> T Sub(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 - arg2;
	}
	template <typename T> T Mul(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 * arg2;
	}
	template <typename T> T Div(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 / arg2;
	}
	template <typename T> T Neg(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		return -arg1;
	}
	float Rcp(float v) { return 1.0f / v; };
	template <typename T> T Rcp(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, Rcp>(aContext);
	}
	float Inc(float v) { return v + 1.0f; };
	template <typename T> T Inc(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, Inc>(aContext);
	}
	float Dec(float v) { return v - 1.0f; };
	template <typename T> T Dec(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, Dec>(aContext);
	}
	
	// relational operators
	template <typename T> bool Greater(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 > arg2;
	}
	template <typename T> bool GreaterEqual(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 >= arg2;
	}
	template <typename T> bool Less(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 < arg2;
	}
	template <typename T> bool LessEqual(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 <= arg2;
	}
	template <typename T> bool Equal(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 == arg2;
	}
	template <typename T> bool NotEqual(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 != arg2;
	}

	// trigonometric functions
	template <typename T> T Sin(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, sinf>(aContext);
	}
	template <typename T> T Cos(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, cosf>(aContext);
	}
	template <typename T> T Tan(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, tanf>(aContext);
	}
	template <typename T> T Asin(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, asinf>(aContext);
	}
	template <typename T> T Acos(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, acosf>(aContext);
	}
	template <typename T> T Atan(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, atanf>(aContext);
	}
	template <typename T> T Atan2(Context &aContext)
	{
		return ComponentBinary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, float, atan2f>(aContext);
	}

	// hyperbolic functions
	template <typename T> T Sinh(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, sinhf>(aContext);
	}
	template <typename T> T Cosh(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, coshf>(aContext);
	}
	template <typename T> T Tanh(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, tanhf>(aContext);
	}

	// exponential functions
	template <typename T> T Pow(Context &aContext)
	{
		return ComponentBinary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, float, powf>(aContext);
	}
	template <typename T> T Exp(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, expf>(aContext);
	}
	template <typename T> T Log(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, logf>(aContext);
	}
	template <typename T> T Sqrt(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, sqrtf>(aContext);
	}
	template <typename T> T InvSqrt(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, ::InvSqrt>(aContext);
	}

	// common functions
	template <typename T> T Abs(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, fabsf>(aContext);
	}
	float Sign(float v) { return (v == 0) ? (0.0f) : ((v > 0) ? (1.0f) : (-1.0f)); }
	template <typename T> T Sign(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, Sign>(aContext);
	}
	template <typename T> T Floor(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, floorf>(aContext);
	}
	template <typename T> T Ceil(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, ceilf>(aContext);
	}
	float Frac(float v) { return v - floor(v); }
	template <typename T> T Frac(Context &aContext)
	{
		return ComponentUnary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, Frac>(aContext);
	}
	template <typename T> T Mod(Context &aContext)
	{
		return ComponentBinary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, float, fmodf>(aContext);
	}
	template <typename T> T Min(Context &aContext)
	{
		return ComponentBinary<T, ComponentCount<T>::VALUE>::Evaluate<const float &, const float &, const float &, std::min<float> >(aContext);
	}
	template <typename T> T Max(Context &aContext)
	{
		return ComponentBinary<T, ComponentCount<T>::VALUE>::Evaluate<const float &, const float &, const float &, std::max<float> >(aContext);
	}
	template <typename T> T Clamp(Context &aContext)
	{
		return ComponentTernary<T, ComponentCount<T>::VALUE>::Evaluate<const float, const float, const float, const float, ::Clamp<float> >(aContext);
	}
	template <typename T> T Lerp(Context &aContext)
	{
		return Ternary<T, T, T, float>::Evaluate<const T, const T, const T, float, ::Lerp<T> >(aContext);
	}
	float Step(float e, float v) { return v < e ? 0.0f : 1.0f; }
	template <typename T> T Step(Context &aContext)
	{
		return ComponentBinary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, float, Step >(aContext);
	}
	float SmoothStep(float e0, float e1, float v)
	{
		if (v <= e0) return 0.0f;
		if (v >= e1) return 1.0f;
		float t = (v - e0) / (e1 - e0);
		return t * t * (3 - 2 * t);
	}
	template <typename T> T SmoothStep(Context &aContext)
	{
		return ComponentBinary<T, ComponentCount<T>::VALUE>::Evaluate<float, float, float, Step >(aContext);
	}
}


//
// LITERAL EXPRESSION
// returns an embedded constant value
//

// float[width] literal
void ConfigureLiteral(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[])
{
	// process literal data
	for (int i = 0; i < width; ++i)
	{
		float value = defaults[i];
		element->QueryFloatAttribute(names[i], &value);
		buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
	}
}

// typed literal
template<typename T> void ConfigureLiteral(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a constant expression
	Expression::Append(buffer, Expression::Constant<T>);
	ConfigureLiteral(element, buffer, sizeof(T)/sizeof(float), names, defaults);

}


//
// VARIABLE EXPRESSION
// returns the value of a named variable
//

// evaluate float[width] variable
void EvaluateVariable(float value[], int width, DrawItemContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	const Database::Typed<float> &variables = Database::variable.Get(aContext.mId);
	for (int i = 0; i < width; ++i)
		value[i] = variables.Get(name+i);
}

// evaluate typed variable
template <typename T> static const T EvaluateVariable(DrawItemContext &aContext)
{
	T value = T();
	EvaluateVariable(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), aContext);
	return value;
}

// typed variable: attribute-inlined version
template<typename T> void ConfigureInlineVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("variable")));
}

// typed variable: normal version
template<typename T> void ConfigureVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("name")));
}


//
// INTERPOLATOR EXPRESSION
// returns interpolated value based on parameter
//

// evaluate float[width] interpolator
void EvaluateInterpolator(float value[], int width, DrawItemContext &aContext)
{
	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get interpolator value
	const int count = Expression::Read<int>(aContext);
	const float * __restrict keys = reinterpret_cast<const float * __restrict>(aContext.mStream);
	int dummy = 0;
	ApplyInterpolator(value, width, count, keys, aContext.mParam, dummy);

	// advance stream
	aContext.mStream = end;
}

// evaluate typed interpolator
template <typename T> static const T EvaluateInterpolator(DrawItemContext &aContext)
{
	T value = T();
	EvaluateInterpolator(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), aContext);
	return value;
}

// configure float[width] interpolator
static void ConfigureInterpolator(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[])
{
	// process interpolator data
	buffer.push_back(0);
	int start = buffer.size();
	ConfigureInterpolatorItem(element, buffer, width, names, defaults);
	buffer[start - 1] = buffer.size() - start;
}

// configure typed interpolator
template<typename T> void ConfigureInterpolator(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append an interpolator expression
	Expression::Append(buffer, EvaluateInterpolator<T>);
	ConfigureInterpolator(element, buffer, sizeof(T)/sizeof(float), names, defaults);
}


//
// RANDOM EXPRESSION
// returns a random value: average + variance * rand[-1..1]
//

// random [-1..1]
float Rand(void)
{
	return Random::Value(0.0f, 1.0f);
}

// TO DO: float[width] random

// configure typed random
template<typename T> void ConfigureRandom(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// get random count
	int count = 1;
	element->QueryIntAttribute("rand", &count);

	if (count > 0)
	{
		// push add
		Expression::Append(buffer, Expression::Add<T>);
	}

	// push average
	Expression::Append(buffer, Expression::Constant<T>);
	for (int i = 0; i < width; i++)
	{
		char label[64];
		sprintf(label, "%s_avg", names[i]);
		float average = defaults[i];
		element->QueryFloatAttribute(label, &average);
		buffer.push_back(*reinterpret_cast<unsigned int *>(&average));
	}

	if (count > 0)
	{
		// push multiply
		Expression::Append(buffer, Expression::Mul<T>);

		// push variance
		Expression::Append(buffer, Expression::Constant<T>);
		for (int i = 0; i < sizeof(T)/sizeof(float); i++)
		{
			char label[64];
			sprintf(label, "%s_var", names[i]);
			float variance = 0.0f;
			element->QueryFloatAttribute(label, &variance);
			variance /= count;
			buffer.push_back(*reinterpret_cast<unsigned int *>(&variance));
		}

		for (int i = 0; i < count; ++i)
		{
			if (count > 1 && i < count - 1)
			{
				// push add
				Expression::Append(buffer, Expression::Add<T>);
			}

			// push randoms
			Expression::Append(buffer, Expression::Construct<T>);
			for (int w = 0; w < sizeof(T)/sizeof(float); ++w)
				Expression::Append(buffer, Expression::Nullary<float>::Evaluate<float, Rand>);
		}
	}
}

//
// TIME EXPRESSION
//

float EvaluateTime(DrawItemContext &aContext)
{
	return aContext.mParam;
}


//
// TYPE CONVERSION
//
template<typename T, typename A> struct Convert
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Construct<T, A>);
	}
};
template<typename T> struct Convert<T, T>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
	}
};
template<> struct Convert<Vector2, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Vector2, float>);
	}
};
template<> struct Convert<Vector3, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Vector3, float>);
	}
};
template<> struct Convert<Vector4, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Vector4, float>);
	}
};
template<> struct Convert<Color4, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Color4, float>);
	}
};
template<typename T, typename A> void ConfigureConvert(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for type conversion");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Convert<T, A>::Append(buffer);

	// append first argument
	ConfigureExpression<A>(arg1, buffer, names, defaults);
}


//
// UNARY EXPRESSION
// return the result of an expression taking one parameter
//
template<typename T, typename A, typename C> void ConfigureUnary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for unary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A>(arg1, buffer, names, defaults);
}


//
// BINARY EXPRESSION
// return the result of an expression taking two parameters
//
template<typename T, typename A1, typename A2, typename C> void ConfigureBinary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no first argument for binary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		assert(!"no second argument for binary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A1>(arg1, buffer, names, defaults);

	// append second argument
	ConfigureExpression<A2>(arg2, buffer, names, defaults);
}


//
// TERNARY EXPRESSION
// return the result of an expression taking three parameters
//
template<typename T, typename A1, typename A2, typename A3, typename C> void ConfigureTernary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no first argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		assert(!"no second argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg3 = arg1->NextSiblingElement();
	if (!arg3)
	{
		// no third argument: treat element as a literal (HACK)
		assert(!"no third argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A1>(arg1, buffer, names, defaults);

	// append second argument
	ConfigureExpression<A2>(arg2, buffer, names, defaults);

	// append third argument
	ConfigureExpression<A3>(arg3, buffer, names, defaults);
}


//
// VARIADIC EXPRESSION
// binary-to-variadic adapter
//

// configure typed variadic
template<typename T, typename A, typename C> void ConfigureVariadic(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		DebugPrint("no first argument for variadic operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: convert type of first argument (HACK)
		DebugPrint("no second argument for variadic operator: performing type conversion");
		ConfigureConvert<T, A>(arg1, buffer, names, defaults);
		return;
	}

	// rewind
	arg2 = arg1;
	do
	{
		// get next argument
		arg1 = arg2;
		arg2 = arg2->NextSiblingElement();

		// if there is a second argument...
		if (arg2)
		{
			// append the operator
			Expression::Append(buffer, expr);
		}

		// append first argument
		ConfigureExpression<A>(arg1, buffer, names, defaults);
	}
	while (arg2);
}


//
// EXPRESSION
//

// configure an expression
template<typename T> void ConfigureExpression(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == TIXML_SUCCESS)
			overrided = true;
	}

	// configure based on tag name
	switch(Hash(element->Value()))
	{
	case 0x425ed3ca /* "value" */:			ConfigureLiteral<T>(element, buffer, names, data); return;
	case 0x19385305 /* "variable" */:		ConfigureVariable<T>(element, buffer, names, data); return;
	case 0x83588fd4 /* "interpolator" */:	ConfigureInterpolator<T>(element, buffer, names, data); return;
	case 0xa19b8cd6 /* "rand" */:			ConfigureRandom<T>(element, buffer, names, data); return;

	case 0xaa7d7949 /* "extend" */:			ConfigureUnary<const T, float, Expression::Context &>(Expression::Extend<T, float>, element, buffer, sScalarNames, sScalarDefault); return;
//	case 0x40c09172 /* "construct" */:		ConfigureConstruct<T>(element, buffer, names, data); return;
	case 0x5d3c9be4 /* "time" */:			Expression::Append(buffer, EvaluateTime); return;

	case 0x3b391274 /* "add" */:			ConfigureVariadic<T, T>(Expression::Add<T>, element, buffer, names, data); return;
	case 0xdc4e3915 /* "sub" */:			ConfigureVariadic<T, T>(Expression::Sub<T>, element, buffer, names, data); return;
	case 0xeb84ed81 /* "mul" */:			ConfigureVariadic<T, T>(Expression::Mul<T>, element, buffer, names, data); return;
	case 0xe562ab48 /* "div" */:			ConfigureVariadic<T, T>(Expression::Div<T>, element, buffer, names, data); return;
	case 0x3899af41 /* "neg" */:			ConfigureUnary<T, T>(Expression::Neg<T>, element, buffer, names, data); return;
	case 0x31037236 /* "rcp" */:			ConfigureUnary<T, T>(Expression::Rcp<T>, element, buffer, names, data); return;
	case 0xa8e99c47 /* "inc" */:			ConfigureUnary<T, T>(Expression::Inc<T>, element, buffer, names, data); return;
	case 0xc25979d3 /* "dec" */:			ConfigureUnary<T, T>(Expression::Dec<T>, element, buffer, names, data); return;
									
	case 0xe0302a4d /* "sin" */:			ConfigureUnary<T, T>(Expression::Sin<T>, element, buffer, names, data); return;
	case 0xfb8de29c /* "cos" */:			ConfigureUnary<T, T>(Expression::Cos<T>, element, buffer, names, data); return;
	case 0x9cf73498 /* "tan" */:			ConfigureUnary<T, T>(Expression::Tan<T>, element, buffer, names, data); return;
	case 0xfeae7ea6 /* "asin" */:			ConfigureUnary<T, T>(Expression::Asin<T>, element, buffer, names, data); return;
	case 0x3c01df1f /* "acos" */:			ConfigureUnary<T, T>(Expression::Acos<T>, element, buffer, names, data); return;
	case 0x0678cabf /* "atan" */:			ConfigureUnary<T, T>(Expression::Atan<T>, element, buffer, names, data); return;
	case 0xbd26dbf7 /* "atan2" */:			ConfigureBinary<T, T, T>(Expression::Atan2<T>, element, buffer, names, data); return;
									
	case 0x10d2583f /* "sinh" */:			ConfigureUnary<T, T>(Expression::Sinh<T>, element, buffer, names, data); return;
	case 0xf45c461c /* "cosh" */:			ConfigureUnary<T, T>(Expression::Cosh<T>, element, buffer, names, data); return;
	case 0x092855d0 /* "tanh" */:			ConfigureUnary<T, T>(Expression::Tanh<T>, element, buffer, names, data); return;
									
	case 0x58336ad5 /* "pow" */:			ConfigureBinary<T, T, T>(Expression::Pow<T>, element, buffer, names, data); return;
	case 0x72a68728 /* "exp" */:			ConfigureUnary<T, T>(Expression::Exp<T>, element, buffer, names, data); return;
	case 0x3f515151 /* "log" */:			ConfigureUnary<T, T>(Expression::Log<T>, element, buffer, names, data); return;
	case 0x7dee3bcf /* "sqrt" */:			ConfigureUnary<T, T>(Expression::Sqrt<T>, element, buffer, names, data); return;
	case 0x0a6a5946 /* "invsqrt" */:		ConfigureUnary<T, T>(Expression::InvSqrt<T>, element, buffer, names, data); return;
									
	case 0x2a48023b /* "abs" */:			ConfigureUnary<T, T>(Expression::Abs<T>, element, buffer, names, data); return;
	case 0x0cbc8ba4 /* "sign" */:			ConfigureUnary<T, T>(Expression::Sign<T>, element, buffer, names, data); return;
	case 0xb8e70c1d /* "floor" */:			ConfigureUnary<T, T>(Expression::Floor<T>, element, buffer, names, data); return;
	case 0x62e4e208 /* "ceil" */:			ConfigureUnary<T, T>(Expression::Ceil<T>, element, buffer, names, data); return;
	case 0x87aad829 /* "frac" */:			ConfigureUnary<T, T>(Expression::Frac<T>, element, buffer, names, data); return;
	case 0xdf9e7283 /* "mod" */:			ConfigureBinary<T, T, T>(Expression::Mod<T>, element, buffer, names, data); return;
	case 0xc98f4557 /* "min" */:			ConfigureVariadic<T, T>(Expression::Min<T>, element, buffer, names, data); return;
	case 0xd7a2e319 /* "max" */:			ConfigureVariadic<T, T>(Expression::Max<T>, element, buffer, names, data); return;
	case 0xa82efcbc /* "clamp" */:			ConfigureTernary<T, T, T, T>(Expression::Clamp<T>, element, buffer, names, data); return;
	case 0x1e691468 /* "lerp" */:			ConfigureTernary<T, T, T, T>(Expression::Lerp<T>, element, buffer, names, data); return;
	case 0xc7441a0f /* "step" */:			ConfigureBinary<T, T, T>(Expression::Step<T>, element, buffer, names, data); return;
	case 0x95964e7d /* "smoothstep" */:		ConfigureTernary<T, T, T, T>(Expression::SmoothStep<T>, element, buffer, names, data); return;
	default:								assert(false); return;
	}
}

// configure an expression root (the tag hosting the expression)
template <typename T> void ConfigureExpressionRoot(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == TIXML_SUCCESS)
			overrided = true;
	}

	// special case: attribute variable reference
	if (element->Attribute("variable"))
	{
		ConfigureInlineVariable<T>(element, buffer, names, data);
		return;
	}

	// special case: attribute random value
	if (element->Attribute("rand"))
	{
		ConfigureRandom<T>(element, buffer, names, data);
		return;
	}

	// special case: embedded interpolator keyframes
	if (element->FirstChildElement("key"))
	{
		ConfigureInterpolator<T>(element, buffer, names, data);
		return;
	}

	// special case: no child elements
	if (!element->FirstChildElement())
	{
		// push literal data
		ConfigureLiteral<T>(element, buffer, names, data);
		return;
	}

	// for each child node...
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		// recurse on child
		ConfigureExpression<T>(child, buffer, names, data);
	}
}

void ConfigureFloatData(const TiXmlElement *element, std::vector<unsigned int> &buffer)
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

void ConfigureVariableOperator(const TiXmlElement *element, std::vector<unsigned int> &buffer, DrawlistOp op, bool drawdata)
{
	unsigned int name = Hash(element->Attribute("name"));
	unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char * const *names;
	const float *data;
	GetTypeData(type, width, names, data);

	buffer.push_back(op);
	buffer.push_back(name);
	buffer.push_back(width);
	if (drawdata)
	{
		switch (width)
		{
		case 1: ConfigureExpressionRoot<float>(element, buffer, names, data); break;
		case 2: ConfigureExpressionRoot<Vector2>(element, buffer, names, data); break;
		case 3: ConfigureExpressionRoot<Vector3>(element, buffer, names, data); break;
		case 4: ConfigureExpressionRoot<Vector4>(element, buffer, names, data); break;
		}
	}
}

void ConfigurePrimitive(const TiXmlElement *element, std::vector<unsigned int> &buffer, GLenum mode)
{
	buffer.push_back(DO_glBegin);
	buffer.push_back(mode);
	ConfigureDrawItems(element, buffer);
	buffer.push_back(DO_glEnd);
}

void ConfigureArray(const TiXmlElement *element, std::vector<unsigned int> &buffer, DrawlistOp op, size_t size, size_t stride)
{
	buffer.push_back(op);
	if (size)
		buffer.push_back(size);
	buffer.push_back(stride);

	buffer.push_back(0);
	int start = buffer.size();
	ConfigureFloatData(element, buffer);
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

void ConfigureDrawItem(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x974c9474 /* "pushmatrix" */:
		{
			buffer.push_back(DO_glPushMatrix);
			ConfigureDrawItems(element, buffer);
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
			ConfigureDrawItems(element, buffer);
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
			ConfigureDrawItems(element, buffer);
			buffer.push_back(DO_glPopClientAttrib);
		}
		break;

	case 0xad0ecfd5 /* "translate" */:
		{
			buffer.push_back(DO_glTranslatef);
			ConfigureExpressionRoot<Vector3>(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			buffer.push_back(DO_glRotatef);
			ConfigureExpressionRoot<float>(element, buffer, sRotationNames, sRotationDefault);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			buffer.push_back(DO_glScalef);
			ConfigureExpressionRoot<Vector3>(element, buffer, sScaleNames, sScaleDefault);
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
			buffer.push_back(DO_glVertex3fv);
			ConfigureExpressionRoot<Vector3>(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xe68b9c52 /* "normal" */:
		{
			buffer.push_back(DO_glNormal3fv);
			ConfigureExpressionRoot<Vector3>(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			buffer.push_back(DO_glColor4fv);
			ConfigureExpressionRoot<Color4>(element, buffer, sColorNames, sColorDefault);
		}
		break;

	case 0x090aa9ab /* "index" */:
		{
			buffer.push_back(DO_glIndexf);
			ConfigureExpressionRoot<float>(element, buffer, sIndexNames, sIndexDefault);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			buffer.push_back(DO_glTexCoord2fv);
			ConfigureExpressionRoot<Vector2>(element, buffer, sTexCoordNames, sTexCoordDefault);
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
			ConfigurePrimitive(element, buffer, GL_POINTS);
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			ConfigurePrimitive(element, buffer, GL_LINES);
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			ConfigurePrimitive(element, buffer, GL_LINE_LOOP);
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			ConfigurePrimitive(element, buffer, GL_LINE_STRIP);
		}
		break;

	case 0xd8a57342 /* "triangles" */:
		{
			ConfigurePrimitive(element, buffer, GL_TRIANGLES);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			ConfigurePrimitive(element, buffer, GL_TRIANGLE_STRIP);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			ConfigurePrimitive(element, buffer, GL_TRIANGLE_FAN);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			ConfigurePrimitive(element, buffer, GL_QUADS);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			ConfigurePrimitive(element, buffer, GL_QUAD_STRIP);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			ConfigurePrimitive(element, buffer, GL_POLYGON);
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

	case 0x23e2c68e /* "calldynamiclist" */:
		{
			// hacktastic!
			const char *name = element->Attribute("name");
			if (name)
			{
				const std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Get(Hash(name));
				if (drawlist.size())
				{
					for (size_t i = 0; i < drawlist.size(); ++i)
					{
						buffer.push_back(drawlist[i]);
					}
				}
			}
		}
		break;

	case 0xdf3cf9c0 /* "dynamicdrawlist" */:
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				// process draw items
				unsigned int id = Hash(name);
				std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Open(id);
				ConfigureDrawItems(element, drawlist);
				Database::dynamicdrawlist.Close(id);
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
			ConfigureDrawItems(element, drawlist);
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

			ConfigureArray(element, buffer, DO_glVertexPointer, size, stride);
		}
		break;

	case 0x81491d33 /* "normalarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glNormalPointer, 0, stride);
		}
		break;

	case 0xcce5b995 /* "colorarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glColorPointer, size, stride);
		}
		break;

	case 0x664ead80 /* "indexarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glIndexPointer, 0, stride);
		}
		break;

	case 0x91aa3148 /* "texcoordarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glTexCoordPointer, size, stride);
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
			ConfigureDrawItems(element, buffer);
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
			int repeat = 0;
			element->QueryIntAttribute("repeat", &repeat);

			buffer.push_back(DO_Block);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&start));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&length));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&scale));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&repeat));

			buffer.push_back(0);
			int size = buffer.size();
			ConfigureDrawItems(element, buffer);
			buffer[size-1] = buffer.size() - size;
		}
		break;

	case 0xc6270703 /* "set" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Set, true);
		}
		break;

	case 0x3b391274 /* "add" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Add, true);
		}
		break;

	case 0xdc4e3915 /* "sub" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Sub, true);
		}
		break;

	case 0xeb84ed81 /* "mul" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Mul, true);
		}
		break;

	case 0xe562ab48 /* "div" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Div, true);
		}
		break;

	case 0xc98f4557 /* "min" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Min, true);
		}
		break;

	case 0xd7a2e319 /* "max" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Max, true);
		}
		break;

	case 0x3deb1461 /* "swizzle" */:
		{
			unsigned int name = Hash(element->Attribute("name"));
			unsigned int type = Hash(element->Attribute("type"));
			int width;
			const char * const *names;
			const float *data;
			GetTypeData(type, width, names, data);

			buffer.push_back(DO_Swizzle);
			buffer.push_back(name);
			buffer.push_back(width);

			for (int i = 0; i < width; ++i)
			{
				unsigned int map;
				const char *attrib = element->Attribute(names[i]);
				if (attrib == NULL)
				{
					map = ~0U;
				}
				else if (isdigit(attrib[0]))
				{
					map = attrib[0] - '0';
				}
				else if (unsigned int hash = Hash(attrib))
				{
					for (int j = 0; j < width; ++j)
					{
						if (Hash(names[j]) == hash)
						{
							map = j;
							break;
						}
					}
				}
				buffer.push_back(map);
			}
		}
		break;

	case 0x5c6e1222 /* "clear" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Clear, false);
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
			ConfigureDrawItems(element, buffer);
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
			ConfigureDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;
#endif

	default:
		break;
	}
}

void ConfigureDrawItems(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureDrawItem(child, buffer);
	}
}

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

typedef void (* VariableOperator)(float &, float);

bool EvaluateVariableOperator(DrawItemContext &aContext, VariableOperator op)
{
	unsigned int name = *aContext.mStream++;
	int width = *aContext.mStream++;
	assert(width <= 4);
	Vector4 value;
	switch(width)
	{
	case 1: value = Expression::Evaluate<float>(aContext); break;
	case 2: value = Expression::Evaluate<Vector2>(aContext); break;
	case 3: value = Expression::Evaluate<Vector3>(aContext); break;
	case 4: value = Expression::Evaluate<Vector4>(aContext); break;
	}
	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	for (int i = 0; i < width; i++)
	{
		float &v = variables.Open(name+i);
		op(v, value[i]);
		variables.Close(name+i);
	}
	Database::variable.Close(aContext.mId);
	return true;
}

void VariableOperatorSet(float &v, float data)
{
	v = data;
}

void VariableOperatorAdd(float &v, float data)
{
	v += data;
}

void VariableOperatorSub(float &v, float data)
{
	v -= data;
}

void VariableOperatorMul(float &v, float data)
{
	v *= data;
}

void VariableOperatorDiv(float &v, float data)
{
	v /= data;
}

void VariableOperatorMin(float &v, float data)
{
	v = std::min(v, data);
}

void VariableOperatorMax(float &v, float data)
{
	v = std::max(v, data);
}


#pragma optimize( "t", on )
void ExecuteDrawItems(const unsigned int buffer[], size_t count, float param, unsigned int id)
{
	DrawItemContext context;
	context.mStream = buffer;
	context.mParam = param;
	context.mId = id;
	
	// HACK: 
	while (context.mStream < buffer + count)
	{
		switch (*context.mStream++)
		{
		case DO_glArrayElement:
			glArrayElement(*context.mStream++);
			break;

		case DO_glBegin:
			glBegin(GLenum(*context.mStream++));
			break;

		case DO_glBindTexture:
			glBindTexture(context.mStream[0], context.mStream[1]);
			context.mStream += 2;
			break;

		case DO_glBlendFunc:
			glBlendFunc(context.mStream[0], context.mStream[1]);
			context.mStream += 2;
			break;

		case DO_glCallList:
			glCallList(GLuint(*context.mStream++));
			break;

		case DO_glColor4fv:
			{
				Color4 value(Expression::Evaluate<Color4>(context));
				glColor4fv(&value.r);
			}
			break;

		case DO_glColorPointer:
			{
				GLint size = *context.mStream++;
				GLsizei stride = *context.mStream++;
				size_t count = *context.mStream++;
				glColorPointer(size, GL_FLOAT, stride, &*context.mStream);
				context.mStream += count;
			}
			break;

		case DO_glDisable:
			glDisable(*context.mStream++);
			break;

		case DO_glDisableClientState:
			glDisableClientState(GLenum(*context.mStream++));
			break;

		case DO_glDrawArrays:
			{
				GLenum mode = *context.mStream++;
				GLint first = *context.mStream++;
				size_t count = *context.mStream++;
				glDrawArrays(mode, first, count);
			}
			break;

		case DO_glDrawElements:
			{
				GLenum mode = *context.mStream++;
				GLsizei count = *context.mStream++;
				glDrawElements(mode, count, GL_UNSIGNED_SHORT, &*context.mStream);
				context.mStream += count*sizeof(unsigned short)/sizeof(unsigned int);
			}
			break;

		case DO_glEdgeFlag:
			glEdgeFlag(*context.mStream++ != 0);
			break;

		case DO_glEdgeFlagPointer:
			{
				GLsizei stride = *context.mStream++;
				size_t count = *context.mStream++;
				glEdgeFlagPointer(stride, &*context.mStream);
				context.mStream += count*sizeof(bool)/sizeof(unsigned int);
			}
			break;

		case DO_glEnable:
			glEnable(*context.mStream++);
			break;

		case DO_glEnableClientState:
			glEnableClientState(GLenum(*context.mStream++));
			break;

		case DO_glEnd:
			glEnd();
			break;

		case DO_glIndexf:
			{
				float value(Expression::Evaluate<float>(context));
				glIndexf(value);
			}
			break;

		case DO_glIndexPointer:
			{
				GLsizei stride = *context.mStream++;
				size_t count = *context.mStream++;
				glIndexPointer(GL_FLOAT, stride, &*context.mStream);
				context.mStream += count;
			}
			break;

		case DO_glLoadIdentity:
			glLoadIdentity();
			break;

		case DO_glLoadMatrixf:
			glLoadMatrixf(reinterpret_cast<const GLfloat *>(&*context.mStream));
			context.mStream+=16;
			break;

		case DO_glMultMatrixf:
			glMultMatrixf(reinterpret_cast<const GLfloat *>(&*context.mStream));
			context.mStream+=16;
			break;

		case DO_glNormal3fv:
			{
				Vector3 value(Expression::Evaluate<Vector3>(context));
				glNormal3fv(&value.x);
			}
			break;

		case DO_glNormalPointer:
			{
				GLsizei stride = *context.mStream++;
				size_t count = *context.mStream++;
				glNormalPointer(GL_FLOAT, stride, &*context.mStream);
				context.mStream += count;
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
			glPushAttrib(GLbitfield(*context.mStream++));
			break;

		case DO_glPushClientAttrib:
			glPushClientAttrib(GLbitfield(*context.mStream++));
			break;

		case DO_glPushMatrix:
			glPushMatrix();
			break;

		case DO_glRotatef:
			{
				float value(Expression::Evaluate<float>(context));
				glRotatef(value, 0, 0, 1);
			}
			break;

		case DO_glScalef:
			{
				Vector3 value(Expression::Evaluate<Vector3>(context));
				glScalef(value.x, value.y, value.z);
			}
			break;

		case DO_glTexCoord2fv:
			{
				Vector2 value(Expression::Evaluate<Vector2>(context));
				glTexCoord2fv(&value.x);
			}
			break;

		case DO_glTexCoordPointer:
			{
				GLint size = *context.mStream++;
				GLsizei stride = *context.mStream++;
				size_t count = *context.mStream++;
				glTexCoordPointer(size, GL_FLOAT, stride, &*context.mStream);
				context.mStream += count;
			}
			break;

		case DO_glTranslatef:
			{
				Vector3 value(Expression::Evaluate<Vector3>(context));
				glTranslatef(value.x, value.y, value.z);
			}
			break;

		case DO_glVertex3fv:
			{
				Vector3 value(Expression::Evaluate<Vector3>(context));
				glVertex3fv(&value[0]);
			}
			break;

		case DO_glVertexPointer:
			{
				GLint size = *context.mStream++;
				GLsizei stride = *context.mStream++;
				size_t count = *context.mStream++;
				glVertexPointer(size, GL_FLOAT, stride, &*context.mStream);
				context.mStream += count;
			}
			break;

		case DO_Repeat:
			{
				int repeat = *context.mStream++;
				size_t size = *context.mStream++;
				for (int i = 0; i < repeat; i++)
					ExecuteDrawItems(context.mStream, size, param, id);
				context.mStream += size;
			}
			break;

		case DO_Block:
			{
				float start = *reinterpret_cast<const float *>(context.mStream++);
				float length = *reinterpret_cast<const float *>(context.mStream++);
				float scale = *reinterpret_cast<const float *>(context.mStream++);
				int repeat = *reinterpret_cast<const int *>(context.mStream++);
				unsigned int size = *context.mStream++;
				float t = param - start;
				if (t >= 0.0f && length > 0.0f)
				{
					int loop = xs_FloorToInt(t / length);
					if (repeat < 0 || loop <= repeat)
					{
						t -= loop * length;
						t *= scale;
						ExecuteDrawItems(context.mStream, size, t, id);
					}
				}
				context.mStream += size;
			}
			break;

		case DO_Set:
			EvaluateVariableOperator(context, VariableOperatorSet);
			break;

		case DO_Add:
			EvaluateVariableOperator(context, VariableOperatorAdd);
			break;

		case DO_Sub:
			EvaluateVariableOperator(context, VariableOperatorSub);
			break;

		case DO_Mul:
			EvaluateVariableOperator(context, VariableOperatorMul);
			break;

		case DO_Div:
			EvaluateVariableOperator(context, VariableOperatorDiv);
			break;

		case DO_Min:
			EvaluateVariableOperator(context, VariableOperatorMin);
			break;

		case DO_Max:
			EvaluateVariableOperator(context, VariableOperatorMax);
			break;

		case DO_Swizzle:
			{
				unsigned int name = *context.mStream++;
				int width = *context.mStream++;
				Database::Typed<float> &variables = Database::variable.Open(id);
				float *temp = static_cast<float *>(_alloca(width * sizeof(float)));
				for (int i = 0; i < width; i++)
					temp[i] = variables.Get(name + *context.mStream++);
				for (int i = 0; i < width; i++)
					variables.Put(name + i, temp[i]);
				Database::variable.Close(id);
			}
			break;

		case DO_Clear:
			{
				unsigned int name = *context.mStream++;
				int width = *context.mStream++;
				Database::Typed<float> &variables = Database::variable.Open(id);
				for (int i = 0; i < width; i++)
					variables.Delete(name+i);
				Database::variable.Close(id);
			}
			break;

#ifdef DRAWLIST_LOOP
		case DO_Loop:
			{
				unsigned int name = *context.mStream++;
				float from = *reinterpret_cast<const float *>(context.mStream++);
				float to   = *reinterpret_cast<const float *>(context.mStream++);
				float by   = *reinterpret_cast<const float *>(context.mStream++);
				size_t size = *context.mStream++;

				Database::Typed<float> &variables = Database::variable.Open(id);
				if (by > 0)
				{
					for (float value = from; value <= to; value += by)
					{
						variables.Put(name, value);
						ExecuteDrawItems(context.mStream, size, param, id);
					}
				}
				else
				{
					for (float value = from; value >= to; value += by)
					{
						variables.Put(name, value);
						ExecuteDrawItems(context.mStream, size, param, id);
					}
				}
				variables.Delete(name);
				Database::variable.Close(id);

				context.mStream += size;
			}
			break;
#endif

#ifdef DRAWLIST_EMITTER
		case DO_Emitter:
			{
				unsigned int name = Hash(&context.mStream, sizeof(context.mStream));
				int repeat = *context.mStream++;
				float period = *reinterpret_cast<const float *>(context.mStream++);
				float offsetx = *reinterpret_cast<const float *>(context.mStream++);
				float offsety = *reinterpret_cast<const float *>(context.mStream++);
				float offseta = *reinterpret_cast<const float *>(context.mStream++);
				Matrix2 offset(offseta, Vector2(offsetx, offsety));
				size_t size = *context.mStream++;

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
						ExecuteDrawItems(context.mStream, size, t, id);
						glPopMatrix();
					}
				}

				// done with local variables
				Database::variable.Close(id);

				// restore model matrix
				glPushMatrix();
				glLoadMatrixf(m1);

				// advance data pointer
				context.mStream += size;
			}
			break;
#endif

		default:
			DebugPrint("Unrecognized drawlist operation 0x%08x at index %d\n", *(context.mStream-1), context.mStream-buffer);
			break;
		}
	}
}
#pragma optimize( "", on )

void RebuildDrawlists(void)
{
	// for each entry in the drawlist database...
	for (Database::Typed<GLuint>::Iterator itor(&Database::drawlist); itor.IsValid(); ++itor)
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

void RenderDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// skip if not visible
	if (aTime < 0)
		return;

	// get the dynamic draw list
	const std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Get(aId);

	// skip if empty
	if (buffer.empty())
		return;

	// push a transform
	glPushMatrix();

	// load matrix
	glTranslatef(aTransform.p.x, aTransform.p.y, 0);
	glRotatef(aTransform.a*180/float(M_PI), 0.0f, 0.0f, 1.0f);

	// execute the deferred draw list
	ExecuteDrawItems(&buffer[0], buffer.size(), aTime, aId);

	// reset the transform
	glPopMatrix();
};
