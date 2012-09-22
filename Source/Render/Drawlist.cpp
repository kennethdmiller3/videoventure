#include "StdAfx.h"
#include "Drawlist.h"
#include "Variable.h"
#include "Texture.h"
#include "Interpolator.h"
#include "Noise.h"

#include "Expression.h"
#include "ExpressionConfigure.h"

#include "Render.h"
#include "MatrixStack.h"

//
// DEFINES
//

// enable "loop" drawlist element
#define DRAWLIST_LOOP

// how to manage static data
// defined: create hardware static buffers (fast)
// undefined: only create a static data pool (slow)
#define DRAWLIST_STATIC_BUFFER

// floating-point color format
// defined: store color as GLfloat[4] (16 bytes)
// undefined: store color as GLubyte[4] (4 bytes)
//#define DRAWLIST_FLOAT_COLOR

// vertex count threshold for drawing from static buffer
// undefined: always draw from static buffer
// defined: copy and draw from dynamic buffer for batches smaller than this
// setting this too low or too high can reduce rendering speed
// static draw elements has significant fixed overhead cost but no memory copy cost
// 32 +/- 16 seems to work best, with rapid decline below 16 and slow decline above 48
// the best value is probably CPU-, GPU-, and workload-dependent...
// TO DO: get this working properly with GLubyte drawlist colors
#define DRAWLIST_STATIC_THRESHOLD 8

// vertex normals
// defined: enable support for normals
// undefined: disable support for normals
//#define DRAWLIST_NORMALS

// use shader for drawlist?
// defined: use a shader program
// undefined: use fixed-function
//#define DRAWLIST_USE_SHADER

// debug static geometry flush
// defined: full details of VB and IB contents
// undefined: basic statistics
//#define DRAWLIST_DEBUG_FLUSH_STATIC


//
// FORWARD DECLARATIONS
//

// execute a dynamic draw list
static void ExecuteDrawItems(EntityContext &aContext);

#if defined(DRAWLIST_STATIC_THRESHOLD) || !defined(DRAWLIST_STATIC_BUFFER)
// copy elements
void DO_CopyElements(EntityContext &aContext);
#endif

// draw elements
void DO_DrawElements(EntityContext &aContext);

// configure static drawlist
static void ConfigureStatic(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, std::vector<unsigned int> &generate);


//
// DATA TYPES
//

typedef float DLScalar;
const char * const sScalarNames[] = { "value" };
const float sScalarDefault[] = { 0.0f };
const int sScalarWidth = 1;

//typedef Vector3 DLPosition;
typedef __m128 DLPosition;
static const char * const sPositionNames[] = { "x", "y", "z", "w" };
static const float sPositionDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sPositionWidth = 3;

#ifdef DRAWLIST_NORMALS
//typedef Vector3 DLNormal;
typedef __m128 DLNormal;
static const char * const sNormalNames[] = { "x", "y", "z", "w" };
static const float sNormalDefault[] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const int sNormalWidth = 3;
#endif

//typedef Vector3 DLTranslation;
typedef __m128 DLTranslation;
static const char * const sTranslationNames[] = { "x", "y", "z", "w" };
static const float sTranslationDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sTranslationWidth = 3;

typedef float DLRotation;
static const char * const sRotationNames[] = { "angle" };
static const float sRotationDefault[] = { 0.0f };
static const int sRotationWidth = 1;

//typedef Vector3 DLScale;
typedef __m128 DLScale;
static const char * const sScaleNames[] = { "x", "y", "z", "w" };
static const float sScaleDefault[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const int sScaleWidth = 3;

//typedef Color4 DLColor;
typedef __m128 DLColor;
static const char * const sColorNames[] = { "r", "g", "b", "a" };
static const float sColorDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sColorWidth = 4;

//typedef Vector2 DLTexCoord;
typedef __m128 DLTexCoord;
static const char * const sTexCoordNames[] = { "s", "t", "r", "q" };
static const float sTexCoordDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sTexCoordWidth = 2;

static const char * const sMatrixNames[] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15" };
static const float sMatrixDefault[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static const int sMatrixWidth = 16;

void GetTypeData(unsigned int type, int &width, const char * const *&names, const float *&data)
{
	switch (type)
	{
	default:
	case 0xaeebcbdd /* "scalar" */:		names = sScalarNames; data = sScalarDefault; width = sScalarWidth; break;
	case 0x934f4e0a /* "position" */:	names = sPositionNames; data = sPositionDefault; width = sPositionWidth; break;
#ifdef DRAWLIST_NORMALS
	case 0xe68b9c52 /* "normal" */:		names = sNormalNames; data = sNormalDefault; width = sNormalWidth; break;
#endif
	case 0xad0ecfd5 /* "translate" */:	names = sTranslationNames, data = sTranslationDefault; width = sTranslationWidth; break;
	case 0x21ac415f /* "rotation" */:	names = sRotationNames; data = sRotationDefault; width = sRotationWidth; break;
	case 0x82971c71 /* "scale" */:		names = sScaleNames; data = sScaleDefault; width = sScaleWidth; break;
	case 0x3d7e6258 /* "color" */:		names = sColorNames; data = sColorDefault; width = sColorWidth; break;
	case 0xdd612dd3 /* "texcoord" */:	names = sTexCoordNames; data = sTexCoordDefault; width = sTexCoordWidth; break;
	case 0x15c2f8ec /* "matrix" */:		names = sMatrixNames; data = sMatrixDefault; width = sMatrixWidth; break;
	}
}




//
// BUFFER OBJECTS
//

// buffer state
static BufferObject sStaticVertexBuffer;
static BufferObject sStaticIndexBuffer;

// vertex attributes
// TO DO: get rid of this
const int RENDER_MAX_ATTRIB = 16;
extern int sAttribCount;
extern __m128 sAttribValue[RENDER_MAX_ATTRIB];
extern GLuint sAttribBuffer[RENDER_MAX_ATTRIB];
extern GLuint sAttribWidth[RENDER_MAX_ATTRIB];
extern GLenum sAttribType[RENDER_MAX_ATTRIB];
extern GLuint sAttribSize[RENDER_MAX_ATTRIB];
extern GLuint sAttribStride[RENDER_MAX_ATTRIB];
extern GLuint sAttribOffset[RENDER_MAX_ATTRIB];
extern GLuint sAttribDisplace[RENDER_MAX_ATTRIB];

// packed vertex data
extern GLubyte sVertexPacked[RENDER_MAX_ATTRIB*16];

// vertex work buffer format
extern GLuint sVertexWorkFormat;
extern GLuint sVertexWorkSize;

// vertex work buffer
// TO DO: get rid of this
extern float *sVertexWork;
extern size_t sVertexLimit;
extern size_t sVertexUsed;
extern size_t sVertexCount;
extern size_t sVertexBase;

// index work buffer
// TO DO: get rid of this
extern unsigned short *sIndexWork;
extern size_t sIndexLimit;
extern size_t sIndexCount;

struct PackedRenderState
{
	unsigned char mMode;
	unsigned short mFormat;
};


#ifdef DRAWLIST_USE_SHADER
//
// SHADER
//

// basic vertex shader program
// vertex position: transform by modelview and projection
// vertex color: pass through unmodified
static const GLchar * const sBasicVertexShader =
	"#version 130\n"
	"\n"
	"uniform mat4 modelviewproj;\n"
	"\n"
	"in vec3 position;\n"
	"in vec4 color;\n"
	"\n"
	"out vec4 vscolor;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = modelviewproj * vec4(position, 1.0);\n"
	"    vscolor = color;\n"
	"}\n";

// basic fragment shader program
static const GLchar * const sBasicFragmentShader =
	"#version 130\n"
	"\n"
	"in vec4 vscolor;\n"
	"\n"
	"out vec4 fragmentcolor;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"    fragmentcolor = vscolor;\n"
	"}\n";

// shader program
static GLuint sBasicProgramId;
static GLuint sBasicVertexId;
static GLuint sBasicFragmentId;

// uniform locations
static GLint sUniformModelViewProj;

// attribute locations
static GLint sAttribPosition;
#ifdef DRAWLIST_NORMALS
static GLint sAttribNormal;
#endif
static GLint sAttribColor;
static GLint sAttribTexCoord;

#else

// fixed function attributes
static const GLint sAttribPosition = 0;
#ifdef DRAWLIST_NORMALS
static const GLint sNormalPosition = 1;
#endif
static const GLint sAttribColor = 2;
static const GLint sAttribTexCoord = 3;

#endif

// tag appended to generator drawlist name to prevent it from matching a template name:
// this fixes a bug where a generator would be duplicated when a template inherited from
// a template containing a drawlist (e.g. "playershipinvunlerable" from "playership"),
// throwing off the static buffer generation sequence in RebuildDrawlists
static const char * const sGenTag = "!generate";

// expression context for baking static geometry
struct BakeContext : public EntityContext
{
	// output drawlist
	std::vector<unsigned int> *mTarget;

	// active vertex format
	// TO DO: replace with bitfield/bool[] indexed by attrib location
	GLuint mFormat;

	// current vertex component values
#ifdef DRAWLIST_NORMALS
	GLfloat mNormal[4];
#endif
#ifdef DRAWLIST_FLOAT_COLOR
	GLfloat mColor[4];
#else
	GLubyte mColor[4];
#endif
	GLfloat mTexCoord[4];

	// current drawing mode
	GLenum mDrawMode;

	BakeContext(std::vector<unsigned int> *aTarget, const unsigned int *aBuffer, const size_t aSize, float aParam, unsigned int aId, Database::Typed<float> *aVars = NULL)
		: EntityContext(aBuffer, aSize, aParam, aId, aVars)
		, mTarget(aTarget)
		, mFormat(1 << sAttribPosition)
		, mDrawMode(GL_TRIANGLES)
	{
#ifdef DRAWLIST_NORMALS
		// current vertex component values
		memcpy(mNormal, sNormalDefault, sizeof(mNormal));
#endif
#ifdef DRAWLIST_FLOAT_COLOR
		memcpy(mColor, sColorDefault, sizeof(mColor));
#else
		mColor[0] = mColor[1] = mColor[2] = 0;
		mColor[3] = 255;
#endif
		memcpy(mTexCoord, sTexCoordDefault, sizeof(mTexCoord));
	}
};

// flush static geometry
static void FlushStatic(BakeContext &aContext)
{
	if (sVertexCount == 0 && sIndexCount == 0)
		return;
	assert(aContext.mDrawMode == GL_POINTS || sIndexCount > 0);

	// TO DO: use shader program
	// TO DO: set up attributes
	DebugPrint("VB start=%d size=%d count=%d\n", sStaticVertexBuffer.mEnd, sVertexUsed * sizeof(float), sVertexCount);
#ifdef DRAWLIST_DEBUG_FLUSH_STATIC
	const float *vertex = sVertexWork;
	for (size_t i = 0; i < sVertexCount; ++i)
	{
		DebugPrint("\t%d: p(%.2f %.2f %.2f)", i, vertex[0], vertex[1], vertex[2]);
		vertex += 3;
#ifdef DRAWLIST_NORMALS
		if (aContext.mFormat & (1 << sAttribNormal))
		{
			DebugPrint(" n(%.2f %.2f %.2f)", vertex[0], vertex[1], vertex[2]);
			vertex += 3;
		}
#endif
		if (aContext.mFormat & (1 << sAttribColor))
		{
#ifdef DRAWLIST_FLOAT_COLOR
			DebugPrint(" c(%.2f %.2f %.2f %.2f)", vertex[0], vertex[1], vertex[2], vertex[3]);
			vertex += 4;
#else
			const GLubyte *c = reinterpret_cast<const GLubyte *>(&vertex[0]);
			DebugPrint(" c(%d %d %d %d)", c[0], c[1], c[2], c[3]);
			vertex += 1;
#endif
		}
		if (aContext.mFormat & (1 << sAttribTexCoord))
		{
			DebugPrint(" t(%.2f %.2f)", vertex[0], vertex[1]);
			vertex += 2;
		}
		DebugPrint("\n");
	}
#endif

	DebugPrint("IB start=%d size=%d count=%d\n", sStaticIndexBuffer.mEnd, sIndexCount * sizeof(unsigned short), sIndexCount);
#ifdef DRAWLIST_DEBUG_FLUSH_STATIC
	const unsigned short *index = sIndexWork;
	switch (aContext.mDrawMode)
	{
	case GL_POINTS:
		for (size_t i = 0; i < sIndexCount; ++i)
			DebugPrint("\t%d\n", index[i]);
		break;
	case GL_LINES:
		for (size_t i = 0; i < sIndexCount; i += 2)
			DebugPrint("\t%d %d\n", index[i], index[i + 1]);
		break;
	case GL_TRIANGLES:
		for (size_t i = 0; i < sIndexCount; i += 3)
			DebugPrint("\t%d %d %d\n", index[i], index[i + 1], index[i + 2]);
		break;
	default:
		assert(0);
		break;
	}
#endif

#ifdef DRAWLIST_STATIC_BUFFER
	// copy vertex work buffer to the array buffer
	BufferAppendData(sStaticVertexBuffer, sVertexUsed * sizeof(float), sVertexWork);
	BufferAppendData(sStaticIndexBuffer, sIndexCount * sizeof(unsigned short), sIndexWork);
#else
	// save a persistent copy
	memcpy(reinterpret_cast<unsigned char *>(sStaticVertexBuffer.mPersist) + sStaticVertexBuffer.mEnd, sVertexWork, sVertexUsed * sizeof(float));
	memcpy(reinterpret_cast<unsigned char *>(sStaticIndexBuffer.mPersist) + sStaticIndexBuffer.mEnd, sIndexWork, sIndexCount * sizeof(unsigned short));

	// advance end offsets
	sStaticVertexBuffer.mEnd += sVertexUsed * sizeof(float);
	sStaticIndexBuffer.mEnd += sIndexCount * sizeof(unsigned short);
#endif

	// add a draw elements command
	PackedRenderState state;
	state.mMode = unsigned char(aContext.mDrawMode);
	state.mFormat = unsigned short(aContext.mFormat);
#if defined(DRAWLIST_STATIC_BUFFER) && defined(DRAWLIST_STATIC_THRESHOLD)
	if (sVertexCount < DRAWLIST_STATIC_THRESHOLD)
#endif
#if !defined(DRAWLIST_STATIC_BUFFER) || defined(DRAWLIST_STATIC_THRESHOLD)
		Expression::Append(*aContext.mTarget, DO_CopyElements, state, sStaticVertexBuffer.mStart, sVertexCount, sStaticIndexBuffer.mStart, sIndexCount);
#endif
#ifdef DRAWLIST_STATIC_BUFFER
#if defined(DRAWLIST_STATIC_THRESHOLD)
	else
#endif
		Expression::Append(*aContext.mTarget, DO_DrawElements, state, sStaticVertexBuffer.mStart, sVertexCount, sStaticIndexBuffer.mStart, sIndexCount);
#endif

	// reset work buffer
	sVertexUsed = 0;
	sVertexCount = 0;
	sVertexBase = 0;
	sIndexCount = 0;

	// get ready for the next batch
	sStaticVertexBuffer.mStart = sStaticVertexBuffer.mEnd;
	sStaticIndexBuffer.mStart = sStaticIndexBuffer.mEnd;
}

namespace Database
{
	Typed<std::vector<unsigned int> > dynamicdrawlist(0xdf3cf9c0 /* "dynamicdrawlist" */);
	Typed<std::vector<unsigned int> > generatedrawlist(0x7deba542 /* "generatedrawlist" */);
	Typed<std::vector<unsigned int> > drawlist(0xc98b019b /* "drawlist" */);
	//Typed<DrawRecord> drawrecord(0x7e069162 /* "drawrecord" */);

	namespace Loader
	{
		static void DynamicDrawlistConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Open(aId);
			assert(buffer.size() == 0);
			ConfigureDrawItems(aId, element, buffer, false);
			Database::dynamicdrawlist.Close(aId);
		}
		Configure dynamicdrawlistconfigure(0xdf3cf9c0 /* "dynamicdrawlist" */, DynamicDrawlistConfigure);

		static void DrawlistConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// configure static drawlist and generator drawlist
			Database::Key genId(Hash(sGenTag, aId));
			std::vector<unsigned int> &generate = Database::generatedrawlist.Open(genId);
			assert(generate.size() == 0);
			std::vector<unsigned int> &buffer = Database::drawlist.Open(aId);
			assert(buffer.size() == 0);
			ConfigureStatic(aId, element, buffer, generate);
			Database::drawlist.Close(aId);
			Database::generatedrawlist.Close(genId);
		}
		Configure drawlistconfigure(0xc98b019b /* "drawlist" */, DrawlistConfigure);

		static void VariableConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
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
		Configure variableconfigure(0x19385305 /* "variable" */, VariableConfigure);
	}

}


//
// DRAWLIST VARIABLE OPERATOR
//

// component-level operators
typedef void (* VariableOperator)(float &, float);
void VariableOperatorSet(float &v, float data) { v = data; }
void VariableOperatorAdd(float &v, float data) { v += data; }
void VariableOperatorSub(float &v, float data) { v -= data; }
void VariableOperatorMul(float &v, float data) { v *= data; }
void VariableOperatorDiv(float &v, float data) { v /= data; }
void VariableOperatorMin(float &v, float data) { v = std::min(v, data); }
void VariableOperatorMax(float &v, float data) { v = std::max(v, data); }

// evaluate variable operator
bool EvaluateVariableOperator(EntityContext &aContext)
{
	const VariableOperator op = Expression::Read<VariableOperator>(aContext);
	const unsigned int name = Expression::Read<unsigned int>(aContext);
	const int width = Expression::Read<int>(aContext);
	assert(width <= 4);
	__m128 value;
	switch(width)
	{
	case 1: value = _mm_set_ps1(Expression::Evaluate<float>(aContext)); break;
	case 2: //value = Expression::Evaluate<Vector2>(aContext); break;
	case 3: //value = Expression::Evaluate<Vector3>(aContext); break;
	case 4: value = Expression::Evaluate<__m128>(aContext); break;
	}
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	for (int i = 0; i < width; i++)
	{
		float &v = aContext.mVars->Open(name+i);
		op(v, value.m128_f32[i]);
		aContext.mVars->Close(name+i);
	}
//	Database::variable.Close(aContext.mId);
	return true;
}



//
// DRAWLIST OPERATIONS
//

void DO_CallListBake(BakeContext &aContext)
{
	const unsigned int name(Expression::Read<unsigned int>(aContext));

	const std::vector<unsigned int> &generate = Database::generatedrawlist.Get(name);
//	BakeContext context(aContext);
	const unsigned int *stream = aContext.mStream;
	const unsigned int *begin = aContext.mBegin;
	const unsigned int *end = aContext.mEnd;
	aContext.mBegin = &generate[0];
	aContext.mEnd = aContext.mBegin + generate.size();
	aContext.Restart();
	ExecuteDrawItems(aContext);
	aContext.mStream = stream;
	aContext.mBegin = begin;
	aContext.mEnd = end;
}

void DO_DrawMode(EntityContext &aContext)
{
	// get the render state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));

#ifdef DRAWLIST_USE_SHADER
	// use the basic program
	UseProgram(sBasicProgramId);

	// set combined model view projection matrix
	// (if switching back from non-dynamic geometry)
	if (&GetBoundVertexBuffer() != &GetDynamicVertexBuffer())
	{
#ifdef DYNAMIC_GEOMETRY_IN_VIEW_SPACE
		SetUniformMatrix4(sUniformModelViewProj, ProjectionGet());
#else
		ProjectionPush();
		ProjectionMult(ViewGet());
		SetUniformMatrix4(sUniformModelViewProj, ProjectionGet());
		ProjectionPop();
#endif
	}
#else
	// fixed-function
	UseProgram(0);

	// set model view projection matrix
	// (if switching back from non-dynamic geometry)
	if (&GetBoundVertexBuffer() != &GetDynamicVertexBuffer())
#ifdef DYNAMIC_GEOMETRY_IN_VIEW_SPACE
		SetUniformMatrix4(GL_MODELVIEW, IdentityGet());
#else
		SetUniformMatrix4(GL_MODELVIEW, ViewGet());
#endif
#endif

	// set up attributes
	SetAttribFormat(sAttribPosition, sPositionWidth, GL_FLOAT);
#ifdef DRAWLIST_NORMALS
	SetAttribFormat(sAttribNormal, sNormalWidth, GL_FLOAT);
#endif
#ifdef DRAWLIST_FLOAT_COLOR	// always use float for dynamic stuff
	SetAttribFormat(sAttribColor, sColorWidth, GL_FLOAT);
#else
	SetAttribFormat(sAttribColor, sColorWidth, GL_UNSIGNED_BYTE);
#endif
	SetAttribFormat(sAttribTexCoord, sTexCoordWidth, GL_FLOAT);

	// set work format
	// include color attribute even if the state doesn't
	GLuint format = state.mFormat;
	format |= 1 << sAttribColor;
	SetWorkFormat(format);

	// set draw mode
	SetDrawMode(state.mMode);

	// set the vertex base
	sVertexBase = sVertexCount;
}

void DO_DrawModeBake(BakeContext &aContext)
{
	// get the render state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));

	// different draw mode or vertex format is incompatible
	if (state.mMode != aContext.mDrawMode ||
		state.mFormat & ~aContext.mFormat)
	{
		// flush geometry
		FlushStatic(aContext);

		// switch to the requested render state
		aContext.mDrawMode = state.mMode;
		aContext.mFormat |= state.mFormat;
	}

	// set the vertex base
	sVertexBase = sVertexCount;
}

void DO_BindTexture(EntityContext &aContext)
{
	FlushDynamic();

	const GLenum target(Expression::Read<GLenum>(aContext));
	const GLuint texture(Expression::Read<GLuint>(aContext));
	glBindTexture(target, texture);
}

void DO_BindTextureBake(BakeContext &aContext)
{
	FlushStatic(aContext);

	const GLenum target(Expression::Read<GLenum>(aContext));
	const GLuint texture(Expression::Read<GLuint>(aContext));
	Expression::Append(*aContext.mTarget, DO_BindTexture, target, texture);
}

void DO_Color(EntityContext &aContext)
{
	SetAttribValue(sAttribColor, Expression::Evaluate<DLColor>(aContext));
}

void DO_ColorBake(BakeContext &aContext)
{
	aContext.mFormat |= 1 << sAttribColor;
	DLColor color(Expression::Evaluate<DLColor>(aContext));
#ifdef DRAWLIST_FLOAT_COLOR
	_mm_storeu_ps(aContext.mColor, color);
#else
	for (register int i = 0; i < 4; ++i)
		aContext.mColor[i] = GLubyte(Clamp(xs_RoundToInt(color.m128_f32[i] * 255), 0, 255));
#endif
}

void DO_Disable(EntityContext &aContext)
{
	FlushDynamic();
	GLenum cap(Expression::Read<GLenum>(aContext));
	glDisable(cap);
}

void DO_DisableBake(BakeContext &aContext)
{
	FlushStatic(aContext);
	GLenum cap(Expression::Read<GLenum>(aContext));
	Expression::Append(*aContext.mTarget, DO_Disable, cap);
}

#pragma optimize("t", on)
void DO_CopyElements(EntityContext &aContext)
{
#ifdef DRAWLIST_USE_SHADER
	// use the basic program
	UseProgram(sBasicProgramId);

	// set combined model view projection matrix
	// (if switching back from non-dynamic geometry)
	if (&GetBoundVertexBuffer() != &GetDynamicVertexBuffer())
	{
#ifdef DYNAMIC_GEOMETRY_IN_VIEW_SPACE
		SetUniformMatrix4(sUniformModelViewProj, ProjectionGet());
#else
		ProjectionPush();
		ProjectionMult(ViewGet());
		SetUniformMatrix4(sUniformModelViewProj, ProjectionGet());
		ProjectionPop();
#endif
	}
#else
	// fixed function
	UseProgram(0);

	// set model view matrix
	// (if switching back from non-dynamic geometry)
	if (&GetBoundVertexBuffer() != &GetDynamicVertexBuffer())
#ifdef DYNAMIC_GEOMETRY_IN_VIEW_SPACE
		SetUniformMatrix4(GL_MODELVIEW, IdentityGet());
#else
		SetUniformMatrix4(GL_MODELVIEW, ViewGet());
#endif
#endif

	// get rendering state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));
	GLuint vertexOffset(Expression::Read<GLuint>(aContext));
	GLuint vertexCount(Expression::Read<GLuint>(aContext));
	GLuint indexOffset(Expression::Read<GLuint>(aContext));
	GLuint indexCount(Expression::Read<GLuint>(aContext));

	// set up attributes
	SetAttribFormat(sAttribPosition, sPositionWidth, GL_FLOAT);
#ifdef DRAWLIST_NORMALS
	SetAttribFormat(sAttribNormal, sNormalWidth, GL_FLOAT);
#endif
#ifdef DRAWLIST_FLOAT_COLOR	// always use float for dynamic stuff
	SetAttribFormat(sAttribColor, sColorWidth, GL_FLOAT);
#else
	SetAttribFormat(sAttribColor, sColorWidth, GL_UNSIGNED_BYTE);
#endif
	SetAttribFormat(sAttribTexCoord, sTexCoordWidth, GL_FLOAT);

	// get the source format
	GLuint srcformat = state.mFormat;

	// set new work format
	GLuint dstformat = srcformat | GetWorkFormat();
	SetWorkFormat(dstformat);

	// set draw mode
	SetDrawMode(state.mMode);

	assert(sVertexWorkFormat != 0);
	assert(sVertexWorkSize != 0);

	// get the vertex base
	size_t base = GetVertexCount();

	// get data source
	const unsigned char *srcConst = sVertexPacked;
	const unsigned char *srcVertex = reinterpret_cast<const unsigned char *>(intptr_t(sStaticVertexBuffer.mPersist) + vertexOffset);

	// get work data destination
	unsigned char *dstVertex = static_cast<unsigned char *>(AllocVertices(vertexCount));

	// copy and transform vertex data
	DLPosition pos = _mm_loadu_ps(sPositionDefault);
#ifdef DRAWLIST_NORMALS
	DLNormal nrm = _mm_loadu_ps(sNormalDefault);
#endif
	for (size_t i = 0; i < vertexCount; ++i)
	{
		// position array always active
		{
			// copy position
			const size_t dstDisplace = sAttribDisplace[sAttribPosition];
			memcpy(&pos, srcVertex, sPositionWidth * sizeof(float));
			pos = StackTransformPosition(pos);
			memcpy(dstVertex + dstDisplace, &pos, sPositionWidth * sizeof(float));
			srcVertex += sPositionWidth * sizeof(float);
		}

#ifdef DRAWLIST_NORMALS
		// if normal array active...
		if (dstformat & (1 << sAttribNormal))
		{
			// copy normal
			const size_t dstDisplace = sAttribDisplace[sAttribNormal];
			if (srcformat & (1 << sAttribNormal))
			{
				// copy source normal
				memcpy(&nrm, srcVertex, sNormalWidth * sizeof(float));
				srcVertex += sNormalWidth * sizeof(float);
			}
			else
			{
				// copy constant normal
				memcpy(&nrm, srcConst + dstDisplace, sNormalWidth * sizeof(float));
			}
			nrm = StackTransformNormal(nrm);
			memcpy(dstVertex + dstDisplace, &nrm, sNormalWidth * sizeof(float));
		}
#endif

		// if color array active...
		if (dstformat & (1 << sAttribColor))
		{
			// copy color
			const size_t dstDisplace = sAttribDisplace[sAttribColor];
			if (srcformat & (1 << sAttribColor))
			{
				// copy source color
#ifdef DRAWLIST_FLOAT_COLOR
				memcpy(dstVertex + dstDisplace, srcVertex, sColorWidth * sizeof(float));
				srcVertex += sColorWidth * sizeof(float);
#elif 1
				memcpy(dstVertex + dstDisplace, srcVertex, sColorWidth * sizeof(unsigned char));
				srcVertex += sColorWidth * sizeof(unsigned char);
#else
#error
#endif
			}
			else
			{
				// copy constant color
#ifdef DRAWLIST_FLOAT_COLOR
				memcpy(dstVertex + dstDisplace, srcConst + dstDisplace, sColorWidth * sizeof(float));
#elif 1
				memcpy(dstVertex + dstDisplace, srcConst + dstDisplace, sColorWidth * sizeof(unsigned char));
#else
#error
#endif
			}
		}

		// if texcoord array active...
		if (dstformat & (1 << sAttribTexCoord))
		{
			// copy texcoord
			const size_t dstDisplace = sAttribDisplace[sAttribTexCoord];
			if (srcformat & (1 << sAttribTexCoord))
			{
				// copy source color
				memcpy(dstVertex + dstDisplace, srcVertex, sTexCoordWidth * sizeof(float));
				srcVertex += sTexCoordWidth * sizeof(float);
			}
			else
			{
				// copy constant color
				memcpy(dstVertex + dstDisplace, srcConst + dstDisplace, sTexCoordWidth * sizeof(float));
			}
		}

		// advance to the next vertex
		dstVertex += sVertexWorkSize;
	}

	// copy and adjust indices
	const unsigned short *srcIndex = reinterpret_cast<unsigned short *>(intptr_t(sStaticIndexBuffer.mPersist) + indexOffset);
	unsigned short *dstIndex = static_cast<unsigned short *>(AllocIndices(indexCount));
	for (size_t i = 0; i < indexCount; ++i)
	{
		*dstIndex++ = unsigned short(*srcIndex++ + base);
	}
}
#pragma optimize("", on)

#ifdef DRAWLIST_STATIC_BUFFER
extern void SetAttribConstantInternal(GLint aIndex, __m128 aValue);
extern void SetAttribBufferInternal(GLint aIndex, BufferObject &aBuffer, GLuint aStride, GLuint aOffset);

static void SetupStaticAttribs(GLuint aFormat, GLuint aOffset)
{
	// get vertex stride
	size_t vertexsize = 0;
	for (int index = 0; index < sAttribCount; ++index)
	{
		if (aFormat & (1 << index))
		{
			vertexsize += sAttribSize[index];
		}
	}

	// set up attribute pointers and values
	size_t offset = aOffset;
	for (int index = 0; index < sAttribCount; ++index)
	{
		if (aFormat & (1 << index))
		{
			SetAttribBufferInternal(index, sStaticVertexBuffer, vertexsize, offset);
			offset += sAttribSize[index];
		}
		else
		{
			SetAttribConstantInternal(index, sAttribValue[index]);
		}
	}
}

void DO_DrawElements(EntityContext &aContext)
{
	// if not currently in static mode...
	if (IsDynamicActive())
	{
		// flush dynamic geometry
		FlushDynamic();
	}

#ifdef DRAWLIST_USE_SHADER
	// use the basic program
	UseProgram(sBasicProgramId);

	// get combined model view projection matrix
	ProjectionPush();
	ProjectionMult(ViewGet());
	ProjectionMult(StackGet());
	SetUniformMatrix4(sUniformModelViewProj, ProjectionGet());
	ProjectionPop();
#else
	// fixed function
	UseProgram(0);

#ifndef DYNAMIC_GEOMETRY_IN_VIEW_SPACE
	// combine view and world matrix
	GLfloat world[16];
	memcpy(world, StackGet(), sizeof(world));
	StackPush();
	StackLoad(ViewGet());
	StackMult(world);
#endif

	// set model view matrix
	SetUniformMatrix4(GL_MODELVIEW, StackGet());

#ifndef DYNAMIC_GEOMETRY_IN_VIEW_SPACE
	StackPop();
#endif
#endif

	// set up attributes
	SetAttribFormat(sAttribPosition, sPositionWidth, GL_FLOAT);
#ifdef DRAWLIST_NORMALS
	SetAttribFormat(sAttribNormal, sNormalWidth, GL_FLOAT);
#endif
#ifdef DRAWLIST_FLOAT_COLOR	// always use float for dynamic stuff
	SetAttribFormat(sAttribColor, sColorWidth, GL_FLOAT);
#else
	SetAttribFormat(sAttribColor, sColorWidth, GL_UNSIGNED_BYTE);
#endif
	SetAttribFormat(sAttribTexCoord, sTexCoordWidth, GL_FLOAT);

	// get rendering state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));
	GLuint vertexOffset(Expression::Read<GLuint>(aContext));
	GLuint vertexCount(Expression::Read<GLuint>(aContext));
	GLuint indexOffset(Expression::Read<GLuint>(aContext));
	GLuint indexCount(Expression::Read<GLuint>(aContext));

	// set up attributes
	SetupStaticAttribs(state.mFormat, vertexOffset);

	// emit a draw call
	if (indexCount > 0)
	{
		// set index buffer
		SetIndexBuffer(sStaticIndexBuffer);

		// draw indexed primitive
		DrawElements(state.mMode, vertexCount, indexCount, indexOffset);
	}
	else
	{
		// draw non-indexed primitive
		DrawArrays(state.mMode, vertexCount);
	}
}
#endif

void DO_Enable(EntityContext &aContext)
{
	FlushDynamic();
	GLenum cap(Expression::Read<GLenum>(aContext));
	glEnable(cap);
}

void DO_EnableBake(BakeContext &aContext)
{
	FlushStatic(aContext);
	GLenum cap(Expression::Read<GLenum>(aContext));
	Expression::Append(*aContext.mTarget, DO_Enable, cap);
}

void DO_IndexPoints(EntityContext &aContext)
{
	// points don't use indices :)
}

void DO_IndexLines(EntityContext &aContext)
{
	IndexLines(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexLineLoop(EntityContext &aContext)
{
	IndexLineLoop(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexLineStrip(EntityContext &aContext)
{
	IndexLineStrip(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexTriangles(EntityContext &aContext)
{
	IndexTriangles(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexTriangleStrip(EntityContext &aContext)
{
	IndexTriangleStrip(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexTriangleFan(EntityContext &aContext)
{
	IndexTriangleFan(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexQuads(EntityContext &aContext)
{
	IndexQuads(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexQuadStrip(EntityContext &aContext)
{
	IndexQuadStrip(sVertexBase, sVertexCount - sVertexBase);
}

void DO_IndexPolygon(EntityContext &aContext)
{
	IndexPolygon(sVertexBase, sVertexCount - sVertexBase);
}

void DO_LineWidth(EntityContext &aContext)
{
	FlushDynamic();

	const GLfloat width(Expression::Read<GLfloat>(aContext));
	glLineWidth(width);
}

void DO_LineWidthBake(BakeContext &aContext)
{
	FlushStatic(aContext);

	const GLfloat width(Expression::Read<GLfloat>(aContext));
	Expression::Append(*aContext.mTarget, DO_LineWidth, width);
}

void DO_LineWidthWorld(EntityContext &aContext)
{
	FlushDynamic();

	const GLfloat width(Expression::Read<GLfloat>(aContext));
	glLineWidth(width * float(SCREEN_HEIGHT) / VIEW_SIZE);
}

void DO_LineWidthWorldBake(BakeContext &aContext)
{
	FlushStatic(aContext);

	const GLfloat width(Expression::Read<GLfloat>(aContext));
	Expression::Append(*aContext.mTarget, DO_LineWidthWorld, width);
}

void DO_LoadIdentity(EntityContext &aContext)
{
	// glLoadIdentity();
	StackIdentity();
}

void DO_LoadMatrix(EntityContext &aContext)
{
	//glLoadMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	StackLoad(reinterpret_cast<const float *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_MultMatrix(EntityContext &aContext)
{
	//glMultMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	StackMult(reinterpret_cast<const float *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

#ifdef DRAWLIST_NORMALS
void DO_Normal(EntityContext &aContext)
{
	SetAttribValue(sAttribNormal, Expression::Evaluate<DLNormal>(aContext));
}

void DO_NormalBake(BakeContext &aContext)
{
	aContext.mFormat |= 1 << sAttribNormal;
	_mm_storeu_ps(aContext.mNormal, Expression::Evaluate<DLNormal>(aContext));
}
#endif

void DO_PointSize(EntityContext &aContext)
{
	FlushDynamic();

	const GLfloat size(Expression::Read<GLfloat>(aContext));
	glPointSize(size);
}

void DO_PointSizeBake(BakeContext &aContext)
{
	FlushStatic(aContext);

	const GLfloat size(Expression::Read<GLfloat>(aContext));
	Expression::Append(*aContext.mTarget, DO_PointSize, size);
}

void DO_PointSizeWorld(EntityContext &aContext)
{
	FlushDynamic();

	const GLfloat size(Expression::Read<GLfloat>(aContext));
	glPointSize(size * float(SCREEN_HEIGHT) / VIEW_SIZE);
}

void DO_PointSizeWorldBake(BakeContext &aContext)
{
	FlushStatic(aContext);

	const GLfloat size(Expression::Read<GLfloat>(aContext));
	Expression::Append(*aContext.mTarget, DO_PointSizeWorld, size);
}

void DO_PopAttrib(EntityContext &aContext)
{
	FlushDynamic();

	glPopAttrib();
}

void DO_PopAttribBake(BakeContext &aContext)
{
	FlushStatic(aContext);

	Expression::Append(*aContext.mTarget, DO_PopAttrib);
}

void DO_PopMatrix(EntityContext &aContext)
{
	//glPopMatrix();
	StackPop();
}

void DO_PushAttrib(EntityContext &aContext)
{
	FlushDynamic();

	GLbitfield mask(Expression::Read<GLbitfield>(aContext));
	glPushAttrib(mask);
}

void DO_PushAttribBake(BakeContext &aContext)
{
	FlushStatic(aContext);

	GLbitfield mask(Expression::Read<GLbitfield>(aContext));
	Expression::Append(*aContext.mTarget, DO_PushAttrib, mask);
}

void DO_PushMatrix(EntityContext &aContext)
{
	//glPushMatrix();
	StackPush();
}

void DO_Rotate(EntityContext &aContext)
{
	const float value(Expression::Evaluate<DLRotation>(aContext));
	//glRotatef(value, 0, 0, 1);
	StackRotate(value*float(M_PI/180.0f));
}

void DO_Scale(EntityContext &aContext)
{
	const DLScale value(Expression::Evaluate<DLScale>(aContext));
	StackScale(value);
}

void DO_TexCoord(EntityContext &aContext)
{
	SetAttribValue(sAttribTexCoord, Expression::Evaluate<DLTexCoord>(aContext));
}

void DO_TexCoordBake(BakeContext &aContext)
{
	aContext.mFormat |= 1 << sAttribTexCoord;
	_mm_storeu_ps(aContext.mTexCoord, Expression::Evaluate<DLTexCoord>(aContext));
}

void DO_Translate(EntityContext &aContext)
{
	const DLTranslation value(Expression::Evaluate<DLTranslation>(aContext));
	StackTranslate(value);
}

void DO_Vertex(EntityContext &aContext)
{
	SetAttribValue(sAttribPosition, StackTransformPosition(Expression::Evaluate<DLPosition>(aContext)));
	AddVertex();
}

void DO_VertexBake(BakeContext &aContext)
{
	const DLPosition position(StackTransformPosition(Expression::Evaluate<DLPosition>(aContext)));

	// component sizes
	const size_t sizeposition = sPositionWidth * sizeof(float);
#ifdef DRAWLIST_NORMALS
	const size_t sizenormal = ((aContext.mFormat >> sAttribNormal) & 1) * sNormalWidth * sizeof(float);
#endif
#ifdef DRAWLIST_FLOAT_COLOR
	const size_t sizecolor = ((aContext.mFormat >> sAttribColor) & 1) * sColorWidth * sizeof(float);
#else
	const size_t sizecolor = ((aContext.mFormat >> sAttribColor) & 1) * sColorWidth * sizeof(GLubyte);
#endif
	const size_t sizetexcoord = ((aContext.mFormat >> sAttribTexCoord) & 1) * sTexCoordWidth * sizeof(float);

#ifdef DEBUG
	// combined size
	size_t sizevertex = sizeposition
#ifdef DRAWLIST_NORMALS
		+ sizenormal
#endif
		+ sizecolor
		+ sizetexcoord;

	// make sure there's enough room for the data
	if (sVertexUsed + sizevertex / sizeof(float) >= sVertexLimit)
	{
		FlushStatic(aContext);
	}
#endif

	// add data to the work buffer
	// NOTE: this generates interleaved vertex data
	memcpy(sVertexWork + sVertexUsed, &position, sizeposition);
	sVertexUsed += sizeposition / sizeof(float);
#ifdef DRAWLIST_NORMALS
	if (aContext.mFormat & (1 << sAttribNormal))
	{
		const DLNormal normal(StackTransformNormal(_mm_loadu_ps(aContext.mNormal)));
		memcpy(sVertexWork + sVertexUsed, &normal, sizenormal);
		sVertexUsed += sizenormal / sizeof(float);
	}
#endif
	if (aContext.mFormat & (1 << sAttribColor))
	{
		memcpy(sVertexWork + sVertexUsed, &aContext.mColor, sizecolor);
		sVertexUsed += sizecolor / sizeof(float);
	}
	if (aContext.mFormat & (1 << sAttribTexCoord))
	{
		memcpy(sVertexWork + sVertexUsed, &aContext.mTexCoord, sizetexcoord);
		sVertexUsed += sizetexcoord / sizeof(float);
	}
	++sVertexCount;
}

void DO_Repeat(EntityContext &aContext)
{
	const int repeat(Expression::Read<int>(aContext));
	const size_t size(Expression::Read<size_t>(aContext));

	const unsigned int *begin = aContext.mBegin;
	const unsigned int *end = aContext.mEnd;
	aContext.mBegin = aContext.mStream;
	aContext.mEnd = aContext.mBegin + size;

	for (int i = 0; i < repeat; i++)
	{
		ExecuteDrawItems(aContext);
		aContext.Restart();
	}

	aContext.mStream = aContext.mEnd;
	aContext.mBegin = begin;
	aContext.mEnd = end;
}

void DO_Block(EntityContext &aContext)
{
	const float start(Expression::Read<float>(aContext));
	const float length(Expression::Read<float>(aContext));
	const float scale(Expression::Read<float>(aContext));
	const int repeat(Expression::Read<int>(aContext));
	const size_t size(Expression::Read<size_t>(aContext));

	const unsigned int *begin = aContext.mBegin;
	const unsigned int *end = aContext.mEnd;
	const float param = aContext.mParam;
	aContext.mBegin = aContext.mStream;
	aContext.mEnd = aContext.mBegin + size;

	float t = aContext.mParam - start;
	if (t >= 0.0f && length > 0.0f)
	{
		int loop = xs_FloorToInt(t / length);
		if (repeat < 0 || loop <= repeat)
		{
			t -= loop * length;
			t *= scale;
			aContext.mParam = t;
			ExecuteDrawItems(aContext);
		}
	}

	aContext.mStream = aContext.mEnd;
	aContext.mBegin = begin;
	aContext.mEnd = end;
	aContext.mParam = param;
}

void DO_Swizzle(EntityContext &aContext)
{
	const unsigned int name(Expression::Read<unsigned int>(aContext));
	const int width(Expression::Read<int>(aContext));
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	float *temp = static_cast<float *>(_alloca(width * sizeof(float)));
	for (int i = 0; i < width; i++)
		temp[i] = aContext.mVars->Get(name + Expression::Read<unsigned int>(aContext));
	for (int i = 0; i < width; i++)
		aContext.mVars->Put(name + i, temp[i]);
//	Database::variable.Close(aContext.mId);
}

void DO_Clear(EntityContext &aContext)
{
	const unsigned int name(Expression::Read<unsigned int>(aContext));
	const int width(Expression::Read<int>(aContext));
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	for (int i = 0; i < width; i++)
		aContext.mVars->Delete(name+i);
//	Database::variable.Close(aContext.mId);
}

#ifdef DRAWLIST_LOOP
void DO_Loop(EntityContext &aContext)
{
	const unsigned int name = Expression::Read<unsigned int>(aContext);
	const float from = Expression::Read<float>(aContext);
	const float to   = Expression::Read<float>(aContext);
	const float by   = Expression::Read<float>(aContext);
	const size_t size = Expression::Read<size_t>(aContext);

	const unsigned int *begin = aContext.mBegin;
	const unsigned int *end = aContext.mEnd;
	aContext.mBegin = aContext.mStream;
	aContext.mEnd = aContext.mBegin + size;

//		Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	if (by > 0)
	{
		for (float value = from; value <= to; value += by)
		{
			aContext.mVars->Put(name, value);
			ExecuteDrawItems(aContext);
			aContext.Restart();
		}
	}
	else
	{
		for (float value = from; value >= to; value += by)
		{
			aContext.mVars->Put(name, value);
			ExecuteDrawItems(aContext);
			aContext.Restart();
		}
	}
	aContext.mVars->Delete(name);
//		Database::variable.Close(aContext.mId);

	aContext.mStream = aContext.mEnd;
	aContext.mBegin = begin;
	aContext.mEnd = end;
}
#endif

void ConfigureVariableOperator(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, VariableOperator op)
{
	const unsigned int name = Hash(element->Attribute("name"));
	const unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char * const *names;
	const float *data;
	GetTypeData(type, width, names, data);

	Expression::Append(buffer, EvaluateVariableOperator, op, name, width);
	switch (width)
	{
	case 1: Expression::Loader<float>::ConfigureRoot(element, buffer, names, data); break;
	case 2: //Expression::Loader<Vector2>::ConfigureRoot(element, buffer, names, data); break;
	case 3: //Expression::Loader<Vector3>::ConfigureRoot(element, buffer, names, data); break;
	case 4: Expression::Loader<__m128>::ConfigureRoot(element, buffer, names, data); break;
	}
}

void ConfigureVariableClear(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer)
{
	const unsigned int name = Hash(element->Attribute("name"));
	const unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char * const *names;
	const float *data;
	GetTypeData(type, width, names, data);

	Expression::Append(buffer, DO_Clear, name, width);
}

static const GLenum sModeTable[] =
{
	GL_POINTS,		// GL_POINTS
	GL_LINES,		// GL_LINES
	GL_LINES,		// GL_LINE_LOOP
	GL_LINES,		// GL_LINE_STRIP
	GL_TRIANGLES,	// GL_TRIANGLES
	GL_TRIANGLES,	// GL_TRIANGLE_STRIP
	GL_TRIANGLES,	// GL_TRIANGLE_FAN
	GL_TRIANGLES,	// GL_QUADS
	GL_TRIANGLES,	// GL_QUAD_STRIP
	GL_TRIANGLES,	// GL_POLYGON
};

typedef void (*Op)(EntityContext &);
static const Op sIndexOpTable[] =
{
	DO_IndexPoints,			// GL_POINTS
	DO_IndexLines,			// GL_LINES
	DO_IndexLineLoop,		// GL_LINE_LOOP
	DO_IndexLineStrip,		// GL_LINE_STRIP
	DO_IndexTriangles,		// GL_TRIANGLES
	DO_IndexTriangleStrip,	// GL_TRIANGLE_STRIP
	DO_IndexTriangleFan,	// GL_TRIANGLE_FAN
	DO_IndexQuads,			// GL_QUADS
	DO_IndexQuadStrip,		// GL_QUAD_STRIP
	DO_IndexPolygon,		// GL_POLYGON
};

void ConfigurePrimitive(GLenum mode, unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake, BakeContext *context)
{
	GLenum drawmode = sModeTable[mode];
	Op indexop = sIndexOpTable[mode];

	// set draw mode
	// this operator does two things:
	// 1. flush geometry if the mode or active components change
	// 2. set the vertex base for index generation
	if (bake)
		Expression::Append(buffer, DO_DrawModeBake);
	else
		Expression::Append(buffer, DO_DrawMode);

	// reserve space for draw mode render state
	// NOTE: don't save the pointer because resizing the buffer will invalidate it
	size_t start = buffer.size();
	Expression::Alloc(buffer, sizeof(PackedRenderState));

	// HACK: temp bake for collecting active attributes
	BakeContext temp(NULL, NULL, 0, 0, aId);
	if (!context)
	{
		temp.mTarget = &buffer;
		context = &temp;
	}

	// add vertex-generation operations
	// this will set active components
	ConfigureDrawItems(aId, element, buffer, bake, context);

	// fill in the draw mode render state
	PackedRenderState &state = *new (&buffer[start]) PackedRenderState;
	state.mMode = unsigned char(drawmode);
	state.mFormat = unsigned short(context->mFormat);

	// add index operation
	Expression::Append(buffer, indexop);
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

static void ConfigureMatrix(const tinyxml2::XMLElement *element, float m[])
{
	for (int i = 0; i < 16; i++)
	{
		char name[16];
		sprintf(name, "m%d", i);
		m[i] = sMatrixDefault[i];
		element->QueryFloatAttribute(name, &m[i]);
	}
}

#if 0
// transform: pushmatrix, popmatrix, loadidentity, loadmatrix, multmatrix, rotate, translate, scale
// primitive: points, lines, line_loop, line_strip, triangles, triangle_strip, triangle_fan, quads, quad_strip, polygon
// geometry: vertex, normal, color, texcoord
// grouping: drawlist, dynamicdrawlist, repeat, block, loop
// variable: set, add, sub, mul, div, min, max, swizzle, clear
// renderstate: pushattrib, popattrib, bindtexture, texenv

// leave point primitives alone (no need to index)
// convert line primitives to indexed lines
// convert poly primitives to indexed triangles
// combine primitive with previous primitive when possible
// same draw type (points, lines, triangles)
// no intervening renderstate commands; those act as barriers
// static: next vertex format is a subset of the current format
// dynamic: next vertex format matches current format
// base decision on the size of the next block?

// item: ConfigureDynamicDrawItem(const tinyxml2::XMLElement *element, std::vector<unsigned int> &drawlist)
// block: ConfigureDynamicDrawItems(const tinyxml2::XMLElement *element, std::vector<unsigned int> &drawlist)
// transform/primitive/geometry/grouping/variable commands in dynamicdrawlist
// emit geometry to dynamic VB/IB at runtime
// include drawelements and other commands in dynamicdrawlist
// drawlist command: handle = drawlist.count + 1; dummy = drawlist[handle]; generate = generatedrawlist[handle]; ConfigureStaticDrawItems(element, generate, drawlist)
// dynamicdrawlist command: dynamic = dynamicdrawlist[name], ConfigureDynamicDrawItems(element, dynamic)

// item: ConfigureStaticDrawItem(const tinyxml2::XMLElement *element, std::vector<unsigned int> &drawlist, std::vector<unsigned int> &generate)
// block: ConfigureStaticDrawItems(const tinyxml2::XMLElement *element, std::vector<unsigned int> &drawlist, std::vector<unsigned int> &generate)
// transform/primitive/geometry/grouping/variable commands in generatedrawlist
// emit geometry to static VB/IB during configure
// exclude drawelements and other commands from generatedrawlist
// include drawelements and other commands in staticdrawlist
// drawlist command: inline contents ConfigureStaticDrawItems(element, generate, drawlist)
// dynamicdrawlist command: ?

// forbid renderstate command inside primitive
// forbid vertex command outside of primitive
// normal, color, or texcoord command outside of primitive saves value and activates component
#endif

void ConfigureDrawItem(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake, BakeContext *context)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
		//
		// TRANSFORM COMMANDS

	case 0x974c9474 /* "pushmatrix" */:
		{
			Expression::Append(buffer, DO_PushMatrix);
			ConfigureDrawItems(aId, element, buffer, bake, context);
			Expression::Append(buffer, DO_PopMatrix);
		}
		break;

	case 0xad0ecfd5 /* "translate" */:
		{
			Expression::Append(buffer, DO_Translate);
			Expression::Loader<DLTranslation>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			Expression::Append(buffer, DO_Rotate);
			Expression::Loader<DLRotation>::ConfigureRoot(element, buffer, sRotationNames, sRotationDefault);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			Expression::Append(buffer, DO_Scale);
			Expression::Loader<DLScale>::ConfigureRoot(element, buffer, sScaleNames, sScaleDefault);
		}
		break;

	case 0x938fc4f7 /* "loadidentity" */:
		{
			Expression::Append(buffer, DO_LoadIdentity);
		}
		break;

	case 0x7d22a710 /* "loadmatrix" */:
		{
			float m[16];
			ConfigureMatrix(element, m);
			Expression::Append(buffer, DO_LoadMatrix);
			Expression::Append(buffer, m);
		}
		break;

	case 0x3807cb92 /* "multmatrix" */:
		{
			float m[16];
			ConfigureMatrix(element, m);
			Expression::Append(buffer, DO_MultMatrix);
			Expression::Append(buffer, m);
		}
		break;

		//
		// GEOMETRY COMMANDS

	case 0x945367a7 /* "vertex" */:
		{
			if (bake)
				Expression::Append(buffer, DO_VertexBake);
			else
				Expression::Append(buffer, DO_Vertex);
			Expression::Loader<DLPosition>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

#ifdef DRAWLIST_NORMALS
	case 0xe68b9c52 /* "normal" */:
		{
			if (context)
				context->mFormat |= (1 << sAttribNormal);
			if (bake)
				Expression::Append(buffer, DO_NormalBake);
			else
				Expression::Append(buffer, DO_Normal);
			Expression::Loader<DLNormal>::ConfigureRoot(element, buffer, sNormalNames, sNormalDefault);
		}
		break;
#endif

	case 0x3d7e6258 /* "color" */:
		{
			if (context)
				context->mFormat |= 1 << sAttribColor;
			if (bake)
				Expression::Append(buffer, DO_ColorBake);
			else
				Expression::Append(buffer, DO_Color);
			Expression::Loader<DLColor>::ConfigureRoot(element, buffer, sColorNames, sColorDefault);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			if (context)
				context->mFormat |= 1 << sAttribTexCoord;
			if (bake)
				Expression::Append(buffer, DO_TexCoordBake);
			else
				Expression::Append(buffer, DO_TexCoord);
			Expression::Loader<DLTexCoord>::ConfigureRoot(element, buffer, sTexCoordNames, sTexCoordDefault);
		}
		break;

		//
		// PRIMITIVE COMMANDS

	case 0xbc9567c6 /* "points" */:
		{
			float size = 0.0f;
			element->QueryFloatAttribute("size", &size);
			if (size != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PushAttribBake, GL_POINT_BIT);
					Expression::Append(buffer, DO_PointSizeWorldBake, size);
				}
				else
				{
					Expression::Append(buffer, DO_PushAttrib, GL_POINT_BIT);
					Expression::Append(buffer, DO_PointSizeWorld, size);
				}
			}
			ConfigurePrimitive(GL_POINTS, aId, element, buffer, bake, context);
			if (size != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PopAttribBake);
				}
				else
				{
					Expression::Append(buffer, DO_PopAttrib);
				}
			}
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PushAttribBake, GL_LINE_BIT);
					Expression::Append(buffer, DO_LineWidthWorldBake, width);
				}
				else
				{
					Expression::Append(buffer, DO_PushAttrib, GL_LINE_BIT);
					Expression::Append(buffer, DO_LineWidthWorld, width);
				}
			}
			ConfigurePrimitive(GL_LINES, aId, element, buffer, bake, context);
			if (width != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PopAttribBake);
				}
				else
				{
					Expression::Append(buffer, DO_PopAttrib);
				}
			}
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PushAttribBake, GL_LINE_BIT);
					Expression::Append(buffer, DO_LineWidthWorldBake, width);
				}
				else
				{
					Expression::Append(buffer, DO_PushAttrib, GL_LINE_BIT);
					Expression::Append(buffer, DO_LineWidthWorld, width);
				}
			}
			ConfigurePrimitive(GL_LINE_LOOP, aId, element, buffer, bake, context);
			if (width != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PopAttribBake);
				}
				else
				{
					Expression::Append(buffer, DO_PopAttrib);
				}
			}
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PushAttribBake, GL_LINE_BIT);
					Expression::Append(buffer, DO_LineWidthWorldBake, width);
				}
				else
				{
					Expression::Append(buffer, DO_PushAttrib, GL_LINE_BIT);
					Expression::Append(buffer, DO_LineWidthWorld, width);
				}
			}
			ConfigurePrimitive(GL_LINE_STRIP, aId, element, buffer, bake, context);
			if (width != 0.0f)
			{
				if (bake)
				{
					Expression::Append(buffer, DO_PopAttribBake);
				}
				else
				{
					Expression::Append(buffer, DO_PopAttrib);
				}
			}
		}
		break;

	case 0xd8a57342 /* "triangles" */:
		{
			ConfigurePrimitive(GL_TRIANGLES, aId, element, buffer, bake, context);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			ConfigurePrimitive(GL_TRIANGLE_STRIP, aId, element, buffer, bake, context);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			ConfigurePrimitive(GL_TRIANGLE_FAN, aId, element, buffer, bake, context);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			ConfigurePrimitive(GL_QUADS, aId, element, buffer, bake, context);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			ConfigurePrimitive(GL_QUAD_STRIP, aId, element, buffer, bake, context);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			ConfigurePrimitive(GL_POLYGON, aId, element, buffer, bake, context);
		}
		break;

		//
		// RENDERSTATE COMMANDS

	case 0x937cff81 /* "pushattrib" */:
		{
			GLuint mask = 0U;
			for (const tinyxml2::XMLAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
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
				if (attrib->BoolValue())
					mask |= bit;
				else
					mask &= ~bit;
			}

			if (bake)
			{
				Expression::Append(buffer, DO_PushAttribBake, mask);
				ConfigureDrawItems(aId, element, buffer, bake, context);
				Expression::Append(buffer, DO_PopAttribBake);
			}
			else
			{
				Expression::Append(buffer, DO_PushAttrib, mask);
				ConfigureDrawItems(aId, element, buffer, bake, context);
				Expression::Append(buffer, DO_PopAttrib);
			}

		}
		break;

	case 0x4dead571 /* "bindtexture" */:
		{
			if (const char *name = element->Attribute("name"))
			{
				GLuint texture = Database::texture.Get(Hash(name));
				if (texture)
				{
					// bind the texture object
					if (bake)
					{
						Expression::Append(buffer, DO_EnableBake, GL_TEXTURE_2D);
						Expression::Append(buffer, DO_BindTextureBake, GL_TEXTURE_2D, texture);
					}
					else
					{
						Expression::Append(buffer, DO_Enable, GL_TEXTURE_2D);
						Expression::Append(buffer, DO_BindTexture, GL_TEXTURE_2D, texture);
					}
				}
				else
				{
					DebugPrint("Missing texture %s", name);
				}
			}
		}

		//
		// DRAWLIST COMMANDS

	case 0xc98b019b /* "drawlist" */:
		if (bake)
		{
			// inline the drawlist contents
			ConfigureDrawItems(aId, element, buffer, bake, context);
		}
		else
		{
			// create a generator drawlist
			Database::Key handle = Database::generatedrawlist.GetCount() + 1;
			std::vector<unsigned int> &generate = Database::generatedrawlist.Open(handle);
			assert(generate.size() == 0);

			// configure static data
			ConfigureStatic(aId, element, buffer, generate);

			// close generator drawlist
			Database::generatedrawlist.Close(handle);

			// set name to surrounding world item
			Database::name.Put(handle, Database::name.Get(aId));
		}
		break;

	case 0xd2cf6b75 /* "calllist" */:
		{
			if (const char *name = element->Attribute("name"))
			{
				if (bake)
				{
					// find the generator drawlist
					Database::Key id = Hash(sGenTag, Hash(name));
					const std::vector<unsigned int> &generate = Database::generatedrawlist.Get(id);
					if (generate.size())
					{
						// add bake command
						Expression::Append(buffer, DO_CallListBake, id);
					}
					else
					{
						DebugPrint("Missing generator generate %s\n", name);
					}
				}
				else
				{
					// find the drawlist
					Database::Key id = Hash(name);
					const std::vector<unsigned int> &drawlist = Database::drawlist.Get(id);
					if (drawlist.size())
					{
						// append its contents
						buffer.insert(buffer.end(), drawlist.begin(), drawlist.end());
					}
					else
					{
						DebugPrint("Missing drawlist %s\n", name);
					}
				}
			}
		}
		break;

	case 0xdf3cf9c0 /* "dynamicdrawlist" */:
		{
			if (const char *name = element->Attribute("name"))
			{
				// process draw items
				unsigned int id = Hash(name);
				std::vector<unsigned int> &dynamic = Database::dynamicdrawlist.Open(id);
				ConfigureDrawItems(id, element, dynamic, false);
				Database::dynamicdrawlist.Close(id);
			}
		}
		break;

	case 0x23e2c68e /* "calldynamiclist" */:
		{
			// hacktastic!
			if (const char *name = element->Attribute("name"))
			{
				// TO DO: call drawlist at runtime instead of "inlining" it
				const std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Get(Hash(name));
				if (drawlist.size())
				{
					buffer.insert(buffer.end(), drawlist.begin(), drawlist.end());
				}
				else
				{
					DebugPrint("Missing dynamic drawlist %s\n", name);
				}
			}
		}
		break;

		//
		// GROUPING COMMANDS

	case 0xd99ba82a /* "repeat" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);

			Expression::Append(buffer, DO_Repeat, count);
	
			buffer.push_back(0);
			const int start = buffer.size();
			ConfigureDrawItems(aId, element, buffer, bake, context);
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

			Expression::Append(buffer, DO_Block, start, length, scale, repeat);

			buffer.push_back(0);
			int size = buffer.size();
			ConfigureDrawItems(aId, element, buffer, bake, context);
			buffer[size-1] = buffer.size() - size;
		}
		break;

#ifdef DRAWLIST_LOOP
	case 0xddef486b /* "loop" */:
		{
			const unsigned int name = Hash(element->Attribute("name"));
			float from = 0.0f;
			element->QueryFloatAttribute("from", &from);
			float to = 0.0f;
			element->QueryFloatAttribute("to", &to);
			float by = from < to ? 1.0f : -1.0f;
			element->QueryFloatAttribute("by", &by);

			if ((to - from) * by <= 0)
			{
				DebugPrint("loop name=\"%s\" from=\"%f\" to=\"%f\" by=\"%f\" would never terminate\n");
				break;
			}

			Expression::Append(buffer, DO_Loop, name, from, to, by);

			buffer.push_back(0);
			const int start = buffer.size();
			ConfigureDrawItems(aId, element, buffer, bake, context);
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

			Expression::Append(buffer, DO_Emitter, count, period, x, y, a);

			buffer.push_back(0);
			const int start = buffer.size();
			ConfigureDrawItems(aId, element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;
#endif

		//
		// VARIABLE COMMANDS

	case 0xc6270703 /* "set" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorSet);
		}
		break;

	case 0x3b391274 /* "add" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorAdd);
		}
		break;

	case 0xdc4e3915 /* "sub" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorSub);
		}
		break;

	case 0xeb84ed81 /* "mul" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorMul);
		}
		break;

	case 0xe562ab48 /* "div" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorDiv);
		}
		break;

	case 0xc98f4557 /* "min" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorMin);
		}
		break;

	case 0xd7a2e319 /* "max" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorMax);
		}
		break;

	case 0x3deb1461 /* "swizzle" */:
		{
			const unsigned int name = Hash(element->Attribute("name"));
			const unsigned int type = Hash(element->Attribute("type"));
			int width;
			const char * const *names;
			const float *data;
			GetTypeData(type, width, names, data);

			unsigned int *map = static_cast<unsigned int *>(_alloca(width * sizeof(unsigned int)));
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
			}

			Expression::Append(buffer, DO_Swizzle, name, width);
			for (int i = 0; i < width; ++i)
				buffer.push_back(map[i]);
		}
		break;

	case 0x5c6e1222 /* "clear" */:
		{
			ConfigureVariableClear(element, buffer);
		}
		break;

	default:
		DebugPrint("Unknown draw item \"%s\"\n", element->Value());
		break;
	}
}

void ConfigureDrawItems(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake, BakeContext *context)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureDrawItem(aId, child, buffer, bake, context);
	}
}

static void ConfigureStatic(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, std::vector<unsigned int> &generate)
{
	// push matrix stack
	StackPush();

	// load identity
	StackIdentity();

	// initialize work buffer
	assert(sVertexCount == 0);
	sVertexUsed = 0;
	sVertexCount = 0;
	sVertexBase = 0;
	assert(sIndexCount == 0);
	sIndexCount = 0;

	// configure draw items
	BakeContext context(&buffer, NULL, 0, 0.0f, aId);
	ConfigureDrawItems(aId, element, generate, true, &context);

	// bake static geometry
	context.mBegin = &generate[0];
	context.mEnd = context.mBegin + generate.size();
	context.Restart();
	ExecuteDrawItems(context);
	FlushStatic(context);

	// pop stack matrix
	StackPop();
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

#pragma optimize( "t", on )
void ExecuteDrawItems(EntityContext &aContext)
{
	while (aContext.mStream < aContext.mEnd)
		Expression::Evaluate<void>(aContext);
}
#pragma optimize( "", on )

static void InitBasicProgram(void)
{
#ifdef DRAWLIST_USE_SHADER
	// create basic program
	sBasicVertexId = CreateVertexShader(sBasicVertexShader);
	sBasicFragmentId = CreateFragmentShader(sBasicFragmentShader);
	sBasicProgramId = CreateProgram(sBasicVertexId, sBasicFragmentId);

	// get uniform location
	sUniformModelViewProj = glGetUniformLocation(sBasicProgramId, "modelviewproj");

	// get attribute locations
	sAttribPosition = glGetAttribLocation(sBasicProgramId, "position");
#ifdef DRAWLIST_NORMALS
	sAttribNormal = glGetAttribLocation(sBasicProgramId, "normal");
#endif
	sAttribColor = glGetAttribLocation(sBasicProgramId, "color");
	sAttribTexCoord = glGetAttribLocation(sBasicProgramId, "texcoord");
#endif
}

static void CleanupBasicProgram(void)
{
#ifdef DRAWLIST_USE_SHADER
	glDeleteProgram(sBasicProgramId);
	sBasicProgramId = 0;
	glDeleteShader(sBasicFragmentId);
	sBasicFragmentId = 0;
	glDeleteShader(sBasicVertexId);
	sBasicVertexId = 0;
#endif
}

void InitDrawlists(void)
{
	// setup basic program
	InitBasicProgram();

	// set up static vertex buffer
	BufferInit(sStaticVertexBuffer, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
#ifdef DRAWLIST_STATIC_BUFFER
	BufferGen(sStaticVertexBuffer);
	BufferSetData(sStaticVertexBuffer, 256 * 1024, NULL);
#else
	sStaticVertexBuffer.mSize = 256 * 1024;
#endif
	sStaticVertexBuffer.mPersist = calloc(sStaticVertexBuffer.mSize, 1);

	// set up static index buffer
	BufferInit(sStaticIndexBuffer, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
#ifdef DRAWLIST_STATIC_BUFFER
	BufferGen(sStaticIndexBuffer);
	BufferSetData(sStaticIndexBuffer, 64 * 1024, NULL);
#else
	sStaticIndexBuffer.mSize = 64 * 1024;
#endif
	sStaticIndexBuffer.mPersist = calloc(sStaticIndexBuffer.mSize, 1);

	// initialize matrix stack
	StackInit();
}

void PreResetDrawlists(void)
{
	CleanupBasicProgram();

	// delete buffer objects
	BufferCleanup(sStaticVertexBuffer);
	BufferCleanup(sStaticIndexBuffer);

}

void PostResetDrawlists(void)
{
	// setup basic program
	InitBasicProgram();

#ifdef DRAWLIST_STATIC_BUFFER
	// rebuild static vertex buffer
	BufferGen(sStaticVertexBuffer);
	BufferSetData(sStaticVertexBuffer, sStaticVertexBuffer.mSize, sStaticVertexBuffer.mPersist);

	// rebuild static index buffer
	BufferGen(sStaticIndexBuffer);
	BufferSetData(sStaticIndexBuffer, sStaticIndexBuffer.mSize, sStaticIndexBuffer.mPersist);
#endif
}

void CleanupDrawlists(void)
{
	CleanupBasicProgram();

	// delete buffer objects
	BufferCleanup(sStaticVertexBuffer);
	BufferCleanup(sStaticIndexBuffer);

	// free permanent storage
	free(sStaticVertexBuffer.mPersist);
	sStaticVertexBuffer.mPersist = NULL;
	free(sStaticIndexBuffer.mPersist);
	sStaticIndexBuffer.mPersist = NULL;
}

void RenderDynamicDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// skip if not visible
	if (aTime < 0)
		return;

	// get the dynamic draw list
	const std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Get(aId);

	// skip if empty
	if (buffer.empty())
		return;

	// render the drawlist
	EntityContext context(&buffer[0], buffer.size(), aTime, aId);
	RenderDrawlist(context, aTransform);
}

void RenderStaticDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// skip if not visible
	if (aTime < 0)
		return;

	// get the static draw list
	const std::vector<unsigned int> &buffer = Database::drawlist.Get(aId);

	// skip if empty
	if (buffer.empty())
		return;

	// render the drawlist
	EntityContext context(&buffer[0], buffer.size(), aTime, aId);
	RenderDrawlist(context, aTransform);
}

void RenderDrawlist(EntityContext &context, const Transform2 &aTransform)
{
	// push matrix stack
	StackPush();

	// load matrix
	if (aTransform.p.x != 0.0f || aTransform.p.y != 0.0f)
		//glTranslatef(aTransform.p.x, aTransform.p.y, 0);
		StackTranslate(_mm_setr_ps(aTransform.p.x, aTransform.p.y, 0, 1));
	if (aTransform.a != 0.0f)
		//glRotatef(aTransform.a*180/float(M_PI), 0.0f, 0.0f, 1.0f);
		StackRotate(aTransform.a);

	// execute the deferred draw list
	ExecuteDrawItems(context);

	// pop matrix stack
	StackPop();
};
