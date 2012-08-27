#include "StdAfx.h"
#include "Drawlist.h"
#include "Variable.h"
#include "Texture.h"
#include "Interpolator.h"
#include "Noise.h"

#include "Expression.h"
#include "ExpressionConfigure.h"

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

// how to copy work buffer to the dynamic buffer
// defined: use glMapBufferRange and copy work data
// undefined: use glBufferData to copy work data
//#define DRAWLIST_DYNAMIC_BUFFER_RANGE

// vertex count threshold for drawing from static buffer
// undefined: always draw from static buffer
// defined: copy and draw from dynamic buffer for batches smaller than this
// setting this too low or too high can reduce rendering speed
// static draw elements has significant fixed overhead cost but no memory copy cost
// 32 +/- 16 seems to work best, with rapid decline below 16 and slow decline above 48
// the best value is probably CPU-, GPU-, and workload-dependent...
#define DRAWLIST_STATIC_THRESHOLD 32


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

// set static mode
static bool SetStaticActive(bool aStatic);

// configure static drawlist
static void BeginStatic();
static void EndStatic(unsigned int aId, std::vector<unsigned int> &buffer, const std::vector<unsigned int> &generate);


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

//typedef Vector3 DLNormal;
typedef __m128 DLNormal;
static const char * const sNormalNames[] = { "x", "y", "z", "w" };
static const float sNormalDefault[] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const int sNormalWidth = 3;

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
static const float sColorDefault[] = { 1.0f, 1.0f, 1.0f, 1.0f };
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
	case 0xe68b9c52 /* "normal" */:		names = sNormalNames; data = sNormalDefault; width = sNormalWidth; break;
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

// OpenGL 1.2 functions
typedef void (APIENTRY * PFN_glDrawRangeElements)(GLenum , GLuint , GLuint , GLsizei , GLenum , const GLvoid *);
static PFN_glDrawRangeElements glDrawRangeElements;

// OpenGL 1.5 definitions
#define GL_BUFFER_SIZE 0x8764
#define GL_BUFFER_USAGE 0x8765
#define GL_QUERY_COUNTER_BITS 0x8864
#define GL_CURRENT_QUERY 0x8865
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA
#define GL_BUFFER_ACCESS 0x88BB
#define GL_BUFFER_MAPPED 0x88BC
#define GL_BUFFER_MAP_POINTER 0x88BD
#define GL_STREAM_DRAW 0x88E0
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_DRAW 0x88E4
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA
#define GL_SAMPLES_PASSED 0x8914

// OpenGL 1.5 functions
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef void (APIENTRY * PFN_glGenBuffers)(GLsizei n, GLuint *buffers);
static PFN_glGenBuffers		glGenBuffers;
typedef void (APIENTRY * PFN_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
static PFN_glDeleteBuffers	glDeleteBuffers;
typedef void (APIENTRY * PFN_glBindBuffer)(GLenum target, GLuint buffer);
static PFN_glBindBuffer		glBindBuffer;
typedef void (APIENTRY * PFN_glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
static PFN_glBufferData		glBufferData;
typedef void (APIENTRY * PFN_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
static PFN_glBufferSubData	glBufferSubData;
typedef GLvoid* (APIENTRY * PFN_glMapBuffer)(GLenum , GLenum );
static PFN_glMapBuffer glMapBuffer;
typedef GLboolean (APIENTRY * PFN_glUnmapBuffer)(GLenum );
static PFN_glUnmapBuffer glUnmapBuffer;

// OpenGL map buffer range defines
#define GL_MAP_READ_BIT 0x0001
#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020

// OpenGL map buffer range extension functions
typedef GLvoid* (APIENTRY * PFN_glMapBufferRange)(GLenum , GLintptr , GLsizeiptr , GLbitfield);
static PFN_glMapBufferRange glMapBufferRange;

enum BufferType
{
	BUFFER_DYNAMIC_VERTEX,
	BUFFER_DYNAMIC_INDEX,
	BUFFER_STATIC_VERTEX,
	BUFFER_STATIC_INDEX,
	BUFFER_COUNT
};

// buffer state
struct BufferState
{
	GLuint mSize;
	GLuint mHandle;
	void *mData;
	GLuint mStart;
	GLuint mEnd;
};
static BufferState sBuffer[BUFFER_COUNT];

// vertex work buffer
static float sVertexWork[64 * 1024];
static size_t sVertexUsed;
static size_t sVertexCount;
static size_t sVertexBase;

// index work buffer
static unsigned short sIndexWork[32 * 1024];
static size_t sIndexCount;

// static buffers active?
static bool sStaticActive;
static BufferState *sVertexBuffer;
static BufferState *sIndexBuffer;

// current vertex component values
static DLNormal sNormal;
#ifdef DRAWLIST_FLOAT_COLOR
static DLColor sColor;
#else
static GLubyte sColor[4];
#endif
static DLTexCoord sTexCoord;

// current drawing mode
static GLenum sDrawMode;

// active vertex components
static bool sNormalActive;
static bool sColorActive;
static bool sTexCoordActive;

struct PackedRenderState
{
	unsigned char mMode;
	bool mNormal;
	bool mColor;
	bool mTexCoord;
};

// tag appended to generator drawlist name to prevent it from matching a template name:
// this fixes a bug where a generator would be duplicated when a template inherited from
// a template containing a drawlist (e.g. "playershipinvunlerable" from "playership"),
// throwing off the static buffer generation sequence in RebuildDrawlists
static const char * const sGenTag = "!generate";

// draw elements from the currently active buffer array
static void DrawElements(GLenum aDrawMode, bool aNormalActive, bool aColorActive, bool aTexCoordActive, GLuint aVertexOffset, GLuint aVertexCount, GLuint aIndexOffset, GLuint aIndexCount)
{
	// component sizes
	const size_t sizeposition = sPositionWidth * sizeof(float);
	const size_t sizenormal = aNormalActive * sNormalWidth * sizeof(float);
#ifdef DRAWLIST_FLOAT_COLOR
	const size_t sizecolor = aColorActive * sColorWidth * sizeof(float);
#else
	const size_t sizecolor = aColorActive * sColorWidth * sizeof(GLubyte);
#endif
	const size_t sizetexcoord = aTexCoordActive * sTexCoordWidth * sizeof(float);

	// combined size
	size_t sizevertex = sizeposition + sizenormal + sizecolor + sizetexcoord;

	// set up pointers
	size_t offset = aVertexOffset;
	glVertexPointer(sPositionWidth, GL_FLOAT, sizevertex, reinterpret_cast<GLvoid *>(offset));
	offset += sizeposition;
	sNormalActive = aNormalActive;
	if (aNormalActive)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizevertex, reinterpret_cast<GLvoid *>(offset));
		offset += sizenormal;
	}
	else
	{
		glDisableClientState(GL_NORMAL_ARRAY);
		glNormal3fv(sNormal.m128_f32);
	}
	sColorActive = aColorActive;
	if (aColorActive)
	{
		glEnableClientState(GL_COLOR_ARRAY);
#ifdef DRAWLIST_FLOAT_COLOR
		glColorPointer(sColorWidth, GL_FLOAT, sizevertex, reinterpret_cast<GLvoid *>(offset));
#else
		glColorPointer(sColorWidth, GL_UNSIGNED_BYTE, sizevertex, reinterpret_cast<GLvoid *>(offset));
#endif
		offset += sizecolor;
	}
	else
	{
		glDisableClientState(GL_COLOR_ARRAY);
#ifdef DRAWLIST_FLOAT_COLOR
		glColor4fv(sColor.m128_f32);
#else
		glColor4ubv(sColor);
#endif
	}
	sTexCoordActive = aTexCoordActive;
	if (aTexCoordActive)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(sTexCoordWidth, GL_FLOAT, sizevertex, reinterpret_cast<GLvoid *>(offset));
		offset += sizetexcoord;
	}
	else
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoord2fv(sTexCoord.m128_f32);
	}

	// draw elements
	//glDrawElements(aDrawMode, aIndexCount, GL_UNSIGNED_SHORT, (GLvoid *)aIndexOffset);
	if (aDrawMode == GL_POINTS)
		glDrawArrays(aDrawMode, 0, aVertexCount);
	else
		glDrawRangeElements(aDrawMode, 0, aVertexCount, aIndexCount, GL_UNSIGNED_SHORT, (GLvoid *)aIndexOffset);
}

// flush static geometry
static void FlushStatic(std::vector<unsigned int> &buffer)
{
	if (sVertexCount == 0 && sIndexCount == 0)
		return;
	assert(sDrawMode == GL_POINTS || sIndexCount > 0);

	assert(sStaticActive);

	// if the vertex buffer is full...
	if (sVertexBuffer->mEnd + sVertexUsed * sizeof(float) > sVertexBuffer->mSize)
	{
		DebugPrint("Static vertex buffer is full");
		return;
	}

	// if the element array buffer is full...
	if (sIndexBuffer->mEnd + sIndexCount * sizeof(unsigned short) > sIndexBuffer->mSize)
	{
		DebugPrint("Static index buffer is full");
		return;
	}

	DebugPrint("VB start=%d size=%d count=%d\n", sVertexBuffer->mEnd, sVertexUsed * sizeof(float), sVertexCount);
	const float *vertex = sVertexWork;
	for (size_t i = 0; i < sVertexCount; ++i)
	{
		DebugPrint("\t%d: p(%.2f %.2f %.2f)", i, vertex[0], vertex[1], vertex[2]);
		vertex += 3;
		if (sNormalActive)
		{
			DebugPrint(" n(%.2f %.2f %.2f)", vertex[0], vertex[1], vertex[2]);
			vertex += 3;
		}
		if (sColorActive)
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
		if (sTexCoordActive)
		{
			DebugPrint(" t(%.2f %.2f)", vertex[0], vertex[1]);
			vertex += 2;
		}
		DebugPrint("\n");
	}
	DebugPrint("IB start=%d size=%d count=%d\n", sIndexBuffer->mEnd, sIndexCount * sizeof(unsigned short), sIndexCount);
	const unsigned short *index = sIndexWork;
	switch (sDrawMode)
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

#ifdef DRAWLIST_STATIC_BUFFER
	// copy vertex work buffer to the array buffer
	glBufferSubData(GL_ARRAY_BUFFER, sVertexBuffer->mEnd, sVertexUsed * sizeof(float), sVertexWork);

	// copy index work buffer to the element array buffer
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mEnd, sIndexCount * sizeof(unsigned short), sIndexWork);
#endif

	// save a persistent copy
	memcpy(reinterpret_cast<unsigned char *>(sVertexBuffer->mData) + sVertexBuffer->mEnd, sVertexWork, sVertexUsed * sizeof(float));
	memcpy(reinterpret_cast<unsigned char *>(sIndexBuffer->mData) + sIndexBuffer->mEnd, sIndexWork, sIndexCount * sizeof(float));

	// advance end offsets
	sVertexBuffer->mEnd += sVertexUsed * sizeof(float);
	sIndexBuffer->mEnd += sIndexCount * sizeof(unsigned short);

	// add a draw elements command
	PackedRenderState state;
	state.mMode = unsigned char(sDrawMode);
	state.mNormal = sNormalActive;
	state.mColor = sColorActive;
	state.mTexCoord = sTexCoordActive;
#if defined(DRAWLIST_STATIC_BUFFER) && defined(DRAWLIST_STATIC_THRESHOLD)
	if (sVertexCount < DRAWLIST_STATIC_THRESHOLD)
#endif
#if !defined(DRAWLIST_STATIC_BUFFER) || defined(DRAWLIST_STATIC_THRESHOLD)
		Expression::Append(buffer, DO_CopyElements, state, sVertexBuffer->mStart, sVertexCount, sIndexBuffer->mStart, sIndexCount);
#endif
#ifdef DRAWLIST_STATIC_BUFFER
#if defined(DRAWLIST_STATIC_THRESHOLD)
	else
#endif
		Expression::Append(buffer, DO_DrawElements, state, sVertexBuffer->mStart, sVertexCount, sIndexBuffer->mStart, sIndexCount);
#endif

	// reset work buffer
	sVertexUsed = 0;
	sVertexCount = 0;
	sVertexBase = 0;
	sIndexCount = 0;

	// get ready for the next batch
	sVertexBuffer->mStart = sVertexBuffer->mEnd;
	sIndexBuffer->mStart = sIndexBuffer->mEnd;
}

// flush dynamic geometry
static void FlushDynamic(void)
{
	if (sVertexCount == 0 && sIndexCount == 0)
		return;
	assert(sDrawMode == GL_POINTS || sIndexCount > 0);

	assert(!sStaticActive);

#ifdef DEBUG_FLUSH_DYNAMIC
	DebugPrint("Flush draw=%d n=%d c=%d t=%d\n", sDrawMode, sNormalActive, sColorActive, sTexCoordActive);
	DebugPrint("DVB start=%d size=%d count=%d\n", sVertexBuffer->mStart, sVertexBuffer->mEnd - sVertexBuffer->mStart, sVertexCount);
	const float *vertex = sVertexWork;
	for (size_t i = 0; i < sVertexCount; ++i)
	{
		DebugPrint("\t%d: p(%.2f %.2f %.2f)", i, vertex[0], vertex[1], vertex[2]);
		vertex += 3;
		if (sNormalActive)
		{
			DebugPrint(" n(%.2f %.2f %.2f)", vertex[0], vertex[1], vertex[2]);
			vertex += 3;
		}
		if (sColorActive)
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
		if (sTexCoordActive)
		{
			DebugPrint(" t(%.2f %.2f)", vertex[0], vertex[1]);
			vertex += 2;
		}
		DebugPrint("\n");
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	DebugPrint("DIB start=%d size=%d count=%d\n", sIndexBuffer->mStart, sIndexBuffer->mEnd - sIndexBuffer->mStart, sIndexCount);
	const unsigned short *index = sIndexWork;
	switch (sDrawMode)
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
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
#endif

#ifndef DRAWLIST_DYNAMIC_BUFFER_RANGE

	// copy vertex work buffer to the dynamic vertex buffer
	// (this seems faster than using map buffer range)
	sVertexBuffer->mStart = 0;
	sVertexBuffer->mEnd = sVertexUsed * sizeof(float);
	glBufferData(GL_ARRAY_BUFFER, sVertexBuffer->mEnd, sVertexWork, GL_STREAM_DRAW);

	// copy index work buffer to the dynamic index buffer
	// (this seems faster than using map buffer range)
	sIndexBuffer->mStart = 0;
	sIndexBuffer->mEnd = sIndexCount * sizeof(unsigned short);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mEnd, sIndexWork, GL_STREAM_DRAW);

#else

	// if the vertex buffer is full...
	if (sVertexBuffer->mEnd + sVertexUsed * sizeof(float) > sVertexBuffer->mSize)
	{
		// reset buffer data
		glBufferData(GL_ARRAY_BUFFER, sVertexBuffer->mSize, NULL, GL_STREAM_DRAW);
		sVertexBuffer->mStart = 0;
		sVertexBuffer->mEnd = 0;
	}

	// if the element array buffer is full...
	if (sIndexBuffer->mEnd + sIndexCount * sizeof(unsigned short) > sIndexBuffer->mSize)
	{
		// reset buffer data
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mSize, NULL, GL_STREAM_DRAW);
		sIndexBuffer->mStart = 0;
		sIndexBuffer->mEnd = 0;
	}

	if (glMapBufferRange)
	{
		// copy vertex work buffer to the array buffer
		if (void *data = glMapBufferRange(GL_ARRAY_BUFFER, sVertexBuffer->mEnd, sVertexUsed * sizeof(float), GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_RANGE_BIT|GL_MAP_UNSYNCHRONIZED_BIT))
		{
			memcpy(data, sVertexWork, sVertexUsed * sizeof(float));
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}

		// copy index work buffer to the element array buffer
		if (void *data = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mEnd, sIndexCount * sizeof(unsigned short), GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_RANGE_BIT|GL_MAP_UNSYNCHRONIZED_BIT))
		{
			memcpy(data, sIndexWork, sIndexCount * sizeof(unsigned short));
			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}
	}
	else
	{
		// copy vertex work buffer to the array buffer
		glBufferSubData(GL_ARRAY_BUFFER, sVertexBuffer->mEnd, sVertexUsed * sizeof(float), sVertexWork);

		// copy index work buffer to the element array buffer
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mEnd, sIndexCount * sizeof(unsigned short), sIndexWork);
	}

	// advance end offsets
	sVertexBuffer->mEnd += sVertexUsed * sizeof(float);
	sIndexBuffer->mEnd += sIndexCount * sizeof(unsigned short);
#endif

	// draw elements
	DrawElements(sDrawMode, sNormalActive, sColorActive, sTexCoordActive, sVertexBuffer->mStart, sVertexCount, sIndexBuffer->mStart, sIndexCount);

	// reset work buffer
	sVertexUsed = 0;
	sVertexCount = 0;
	sVertexBase = 0;
	sIndexCount = 0;

	// get ready for the next batch
	sVertexBuffer->mStart = sVertexBuffer->mEnd;
	sIndexBuffer->mStart = sIndexBuffer->mEnd;
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
			BeginStatic();
			ConfigureDrawItems(aId, element, generate, true);
			EndStatic(aId, buffer, generate);
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

#if 0
void DO_Begin(EntityContext &aContext)
{
	glBegin(Expression::Read<GLenum>(aContext));
}
#endif

struct BakeContext : public EntityContext
{
	std::vector<unsigned int> *mTarget;

	BakeContext(std::vector<unsigned int> *aTarget, const unsigned int *aBuffer, const size_t aSize, float aParam, unsigned int aId, Database::Typed<float> *aVars = NULL)
		: EntityContext(aBuffer, aSize, aParam, aId, aVars)
		, mTarget(aTarget)
	{
	}
};

void DO_CallListBake(BakeContext &aContext)
{
	const unsigned int name(Expression::Read<unsigned int>(aContext));

	const std::vector<unsigned int> &generate = Database::generatedrawlist.Get(name);
	BakeContext context(aContext.mTarget, &generate[0], generate.size(), 0.0f, aContext.mId, aContext.mVars);
	ExecuteDrawItems(context);
}

void DO_DrawMode(EntityContext &aContext)
{
	// get the render state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));

	// switch to dynamic mode
	SetStaticActive(false);

	// different draw mode or vertex format is different
	if (sDrawMode != state.mMode ||
		sNormalActive != state.mNormal ||
		sColorActive != state.mColor ||
		sTexCoordActive != state.mTexCoord)
	{
		// flush geometry
		FlushDynamic();

		// switch to the requested render state
#ifdef DEBUG_DYNAMIC_DRAW_MODE_CHANGE
		DebugPrint("DrawMode: mode=%d->%d normal=%d->%d color=%d->%d texcoord=%d->%d\n",
			sDrawMode, state.mMode, 
			sNormalActive, state.mNormal,
			sColorActive, state.mColor,
			sTexCoordActive, state.mTexCoord);
#endif
		sDrawMode = state.mMode;
		sNormalActive = state.mNormal;
		sColorActive = state.mColor;
		sTexCoordActive = state.mTexCoord;
	}

	// set the vertex base
	sVertexBase = sVertexCount;
}

void DO_DrawModeBake(BakeContext &aContext)
{
	// get the render state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));

	// different draw mode or vertex format is different
	if (sDrawMode != state.mMode ||
		sNormalActive < state.mNormal ||
		sColorActive < state.mColor ||
		sTexCoordActive < state.mTexCoord)
	{
		// flush geometry
		FlushStatic(*aContext.mTarget);

		// switch to the requested render state
		sDrawMode = state.mMode;
		sNormalActive |= state.mNormal;
		sColorActive |= state.mColor;
		sTexCoordActive |= state.mTexCoord;
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
	FlushStatic(*aContext.mTarget);

	const GLenum target(Expression::Read<GLenum>(aContext));
	const GLuint texture(Expression::Read<GLuint>(aContext));
	Expression::Append(*aContext.mTarget, DO_BindTexture, target, texture);
}

void DO_BlendFunc(EntityContext &aContext)
{
	FlushDynamic();

	const GLenum sfactor(Expression::Read<GLenum>(aContext));
	const GLenum dfactor(Expression::Read<GLenum>(aContext));
	glBlendFunc(sfactor, dfactor);
}

void DO_BlendFuncBake(BakeContext &aContext)
{
	FlushStatic(*aContext.mTarget);

	const GLenum sfactor(Expression::Read<GLenum>(aContext));
	const GLenum dfactor(Expression::Read<GLenum>(aContext));
	Expression::Append(*aContext.mTarget, DO_BlendFunc, sfactor, dfactor);
}

void DO_Color(EntityContext &aContext)
{
	sColorActive = true;
#ifdef DRAWLIST_FLOAT_COLOR
	sColor = Expression::Evaluate<DLColor>(aContext);
#else
	DLColor c(Expression::Evaluate<DLColor>(aContext));
	sColor[0] = GLubyte(Clamp(xs_RoundToInt(c.m128_f32[0]*255), 0, 255));
	sColor[1] = GLubyte(Clamp(xs_RoundToInt(c.m128_f32[1]*255), 0, 255));
	sColor[2] = GLubyte(Clamp(xs_RoundToInt(c.m128_f32[2]*255), 0, 255));
	sColor[3] = GLubyte(Clamp(xs_RoundToInt(c.m128_f32[3]*255), 0, 255));
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
	FlushStatic(*aContext.mTarget);
	GLenum cap(Expression::Read<GLenum>(aContext));
	Expression::Append(*aContext.mTarget, DO_Disable, cap);
}

void DO_CopyElements(EntityContext &aContext)
{
	// get rendering state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));
	GLuint vertexOffset(Expression::Read<GLuint>(aContext));
	GLuint vertexCount(Expression::Read<GLuint>(aContext));
	GLuint indexOffset(Expression::Read<GLuint>(aContext));
	GLuint indexCount(Expression::Read<GLuint>(aContext));

	// switch to dynamic mode
	SetStaticActive(false);

	// different draw mode or vertex format is different
	if (sDrawMode != state.mMode ||
		sNormalActive < state.mNormal ||
		sColorActive < state.mColor ||
		sTexCoordActive < state.mTexCoord)
	{
		// flush geometry
		FlushDynamic();

		// switch to the requested render state
		sDrawMode = state.mMode;
		sNormalActive |= state.mNormal;
		sColorActive |= state.mColor;
		sTexCoordActive |= state.mTexCoord;
	}

	// component sizes
	const size_t sizeposition = sPositionWidth * sizeof(float);
	const size_t sizedstnormal = sNormalActive * sNormalWidth * sizeof(float);
	const size_t sizesrcnormal = state.mNormal * sNormalWidth * sizeof(float);
#ifdef DRAWLIST_FLOAT_COLOR
	const size_t sizedstcolor = sColorActive * sColorWidth * sizeof(float);
	const size_t sizesrccolor = state.mColor * sColorWidth * sizeof(float);
#else
	const size_t sizedstcolor = sColorActive * sColorWidth * sizeof(GLubyte);
	const size_t sizesrccolor = state.mColor * sColorWidth * sizeof(GLubyte);
#endif
	const size_t sizedsttexcoord = sTexCoordActive * sTexCoordWidth * sizeof(float);
	const size_t sizesrctexcoord = state.mTexCoord * sTexCoordWidth * sizeof(float);

	// combined size
	const size_t sizevertex = sizeposition + sizedstnormal + sizedstcolor + sizedsttexcoord;

	// if a work buffer is full...
	if (sVertexUsed * sizeof(float) + vertexCount * sizevertex > sizeof(sVertexWork) ||
		(sIndexCount + indexCount) * sizeof(unsigned short) > sizeof(sIndexWork))
	{
		// flush geometry
		FlushDynamic();
	}

	// set the vertex base
	sVertexBase = sVertexCount;

	// get static data source
	const float *srcVertex = reinterpret_cast<const float *>(intptr_t(sBuffer[BUFFER_STATIC_VERTEX].mData) + vertexOffset);
	const unsigned short *srcIndex = reinterpret_cast<unsigned short *>(intptr_t(sBuffer[BUFFER_STATIC_INDEX].mData) + indexOffset);

	// get work data destination
	float *dstVertex = sVertexWork + sVertexUsed;
	unsigned short *dstIndex = sIndexWork + sIndexCount;

	// copy and transform vertex data
	DLPosition pos = _mm_set_ps1(1);
	DLNormal nrm = _mm_set_ps1(0);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		memcpy(&pos, srcVertex, sizeposition);
		pos = StackTransformPosition(pos);
		memcpy(dstVertex, &pos, sizeposition);
		//_mm_storeu_ps(dstVertex, StackTransformPosition(_mm_loadu_ps(srcVertex)));
		srcVertex += sizeposition / sizeof(float);
		dstVertex += sizeposition / sizeof(float);

		if (sizedstnormal)
		{
			if (sizesrcnormal)
				memcpy(&nrm, srcVertex, sizesrcnormal);
			else
				nrm = sNormal;
			nrm = StackTransformNormal(nrm);
			memcpy(dstVertex, &nrm, sizedstnormal);
			//_mm_storeu_ps(dstVertex, StackTransformNormal(_mm_loadu_ps(srcVertex)));
		}
		srcVertex += sizesrcnormal / sizeof(float);
		dstVertex += sizedstnormal / sizeof(float);

		if (sizedstcolor)
		{
			if (sizesrccolor)
				memcpy(dstVertex, srcVertex, sizesrccolor);
			else
				memcpy(dstVertex, &sColor, sizedstcolor);
		}
		srcVertex += sizesrccolor / sizeof(float);
		dstVertex += sizedstcolor / sizeof(float);

		if (sizedsttexcoord)
		{
			if (sizesrctexcoord)
				memcpy(dstVertex, srcVertex, sizesrctexcoord);
			else
				memcpy(dstVertex, &sTexCoord, sizedsttexcoord);
		}
		srcVertex += sizesrctexcoord / sizeof(float);
		dstVertex += sizedsttexcoord / sizeof(float);
	}

	// copy and adjust indices
	for (size_t i = 0; i < indexCount; ++i)
	{
		*dstIndex++ = unsigned short(*srcIndex++ + sVertexBase);
	}

	// update vertex work buffer
	sVertexUsed = dstVertex - sVertexWork;
	sVertexCount += vertexCount;

	// update index work buffer
	sIndexCount += indexCount;
}

#ifdef DRAWLIST_STATIC_BUFFER
void DO_DrawElements(EntityContext &aContext)
{
	// if not currently in static mode...
	if (!sStaticActive)
	{
		// flush dynamic geometry
		FlushDynamic();

		// switch to static mode
		SetStaticActive(true);
	}

	// sync opengl matrix
	glPushMatrix();
	glMultMatrixf(StackGetMatrix());

	// get rendering state
	PackedRenderState state(Expression::Read<PackedRenderState>(aContext));
	GLuint vertexOffset(Expression::Read<GLuint>(aContext));
	GLuint vertexCount(Expression::Read<GLuint>(aContext));
	GLuint indexOffset(Expression::Read<GLuint>(aContext));
	GLuint indexCount(Expression::Read<GLuint>(aContext));

	// draw elements using the specified rendering state
	DrawElements(state.mMode, state.mNormal, state.mColor, state.mTexCoord, vertexOffset, vertexCount, indexOffset, indexCount);

	// 
	glPopMatrix();
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
	FlushStatic(*aContext.mTarget);
	GLenum cap(Expression::Read<GLenum>(aContext));
	Expression::Append(*aContext.mTarget, DO_Enable, cap);
}

void DO_IndexPoints(EntityContext &aContext)
{
	// points don't use indices :)
}

void DO_IndexLines(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 0; i < count; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(start + i);
	}
}

void DO_IndexLineLoop(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 0; i < count - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 1);
	}
	sIndexWork[sIndexCount++] = unsigned short(start + count - 1);
	sIndexWork[sIndexCount++] = unsigned short(start);
}

void DO_IndexLineStrip(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 0; i < count - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 1);
	}
}

void DO_IndexTriangles(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 0; i < count; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(start + i);
	}
}

void DO_IndexTriangleStrip(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 0; i < count - 2; ++i)
	{
		const int odd = i & 1;
		sIndexWork[sIndexCount++] = unsigned short(start + i + odd);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 1 - odd);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 2);
	}
}

void DO_IndexTriangleFan(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 1; i < count - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(start);
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 1);
	}
}

void DO_IndexQuads(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 0; i < count; i += 4)
	{
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 1);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 2);
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 2);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 3);
	}
}

void DO_IndexQuadStrip(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 0; i < count - 2; i += 2)
	{
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 1);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 3);
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 3);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 2);
	}
}

void DO_IndexPolygon(EntityContext &aContext)
{
	const GLuint start(sVertexBase);
	const GLuint count(sVertexCount - sVertexBase);
	for (register GLuint i = 1; i < count - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(start);
		sIndexWork[sIndexCount++] = unsigned short(start + i);
		sIndexWork[sIndexCount++] = unsigned short(start + i + 1);
	}
}

void DO_LineWidth(EntityContext &aContext)
{
	FlushDynamic();

	const GLfloat width(Expression::Read<GLfloat>(aContext));
	glLineWidth(width);
}

void DO_LineWidthBake(BakeContext &aContext)
{
	FlushStatic(*aContext.mTarget);

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
	FlushStatic(*aContext.mTarget);

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

void DO_Normal(EntityContext &aContext)
{
	sNormalActive = true;
	sNormal = StackTransformNormal(Expression::Evaluate<DLNormal>(aContext));
}

void DO_PointSize(EntityContext &aContext)
{
	FlushDynamic();

	const GLfloat size(Expression::Read<GLfloat>(aContext));
	glPointSize(size);
}

void DO_PointSizeBake(BakeContext &aContext)
{
	FlushStatic(*aContext.mTarget);

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
	FlushStatic(*aContext.mTarget);

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
	FlushStatic(*aContext.mTarget);

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
	FlushStatic(*aContext.mTarget);

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
	sTexCoordActive = true;
	sTexCoord = Expression::Evaluate<DLTexCoord>(aContext);
}

void DO_TexEnvi(EntityContext &aContext)
{
	FlushDynamic();

	const GLenum pname(Expression::Read<GLint>(aContext));
	const GLint param(Expression::Read<GLint>(aContext));
	glTexEnvi( GL_TEXTURE_ENV, pname, param );
}

void DO_TexEnviBake(BakeContext &aContext)
{
	FlushStatic(*aContext.mTarget);

	const GLenum pname(Expression::Read<GLint>(aContext));
	const GLint param(Expression::Read<GLint>(aContext));
	Expression::Append(*aContext.mTarget, DO_TexEnvi, pname, param);
}

void DO_Translate(EntityContext &aContext)
{
	const DLTranslation value(Expression::Evaluate<DLTranslation>(aContext));
	StackTranslate(value);
}

void DO_Vertex(EntityContext &aContext)
{
	const DLPosition value(StackTransformPosition(Expression::Evaluate<DLPosition>(aContext)));

	// component sizes
	const size_t sizeposition = sPositionWidth * sizeof(float);
	const size_t sizenormal = sNormalActive * sNormalWidth * sizeof(float);
#ifdef DRAWLIST_FLOAT_COLOR
	const size_t sizecolor = sColorActive * sColorWidth * sizeof(float);
#else
	const size_t sizecolor = sColorActive * sColorWidth * sizeof(GLubyte);
#endif
	const size_t sizetexcoord = sTexCoordActive * sTexCoordWidth * sizeof(float);

	// combined size
	size_t sizevertex = sizeposition + sizenormal + sizecolor + sizetexcoord;

	// make sure there's enough room for the data
	assert(sVertexUsed * sizeof(float) + sizevertex < sizeof(sVertexWork));

	// add data to the work buffer
	// NOTE: this generates interleaved vertex data
	memcpy(sVertexWork + sVertexUsed, &value, sizeposition);
	sVertexUsed += sizeposition / sizeof(float);
	if (sNormalActive)
	{
		memcpy(sVertexWork + sVertexUsed, &sNormal, sizenormal);
		sVertexUsed += sizenormal / sizeof(float);
	}
	if (sColorActive)
	{
		memcpy(sVertexWork + sVertexUsed, &sColor, sizecolor);
		sVertexUsed += sizecolor / sizeof(float);
	}
	if (sTexCoordActive)
	{
		memcpy(sVertexWork + sVertexUsed, &sTexCoord, sizetexcoord);
		sVertexUsed += sizetexcoord / sizeof(float);
	}
	++sVertexCount;
}

void DO_Repeat(EntityContext &aContext)
{
	const int repeat(Expression::Read<int>(aContext));
	const size_t size(Expression::Read<size_t>(aContext));
	EntityContext context(aContext.mStream, size, aContext.mParam, aContext.mId, aContext.mVars);
	for (int i = 0; i < repeat; i++)
	{
		ExecuteDrawItems(context);
		context.Restart();
	}
	aContext.mStream += size;
}

void DO_Block(EntityContext &aContext)
{
	const float start(Expression::Read<float>(aContext));
	const float length(Expression::Read<float>(aContext));
	const float scale(Expression::Read<float>(aContext));
	const int repeat(Expression::Read<int>(aContext));
	const size_t size(Expression::Read<size_t>(aContext));
	float t = aContext.mParam - start;
	if (t >= 0.0f && length > 0.0f)
	{
		int loop = xs_FloorToInt(t / length);
		if (repeat < 0 || loop <= repeat)
		{
			t -= loop * length;
			t *= scale;
			EntityContext context(aContext.mStream, size, t, aContext.mId, aContext.mVars);
			ExecuteDrawItems(context);
		}
	}
	aContext.mStream += size;
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

//		Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	EntityContext context(aContext.mStream, size, aContext.mParam, aContext.mId, aContext.mVars);
	if (by > 0)
	{
		for (float value = from; value <= to; value += by)
		{
			context.mVars->Put(name, value);
			ExecuteDrawItems(context);
			context.Restart();
		}
	}
	else
	{
		for (float value = from; value >= to; value += by)
		{
			context.mVars->Put(name, value);
			ExecuteDrawItems(context);
			context.Restart();
		}
	}
	context.mVars->Delete(name);
//		Database::variable.Close(aContext.mId);

	aContext.mStream += size;
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


void ConfigurePrimitive(GLenum mode, unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake)
{
	typedef void (*Op)(EntityContext &);
	Op indexop;
	GLenum drawmode;
	switch (mode)
	{
	default:
		assert(false);
	case GL_POINTS:
		drawmode = GL_POINTS;
		indexop = DO_IndexPoints;
		break;

	case GL_LINES:
		drawmode = GL_LINES;
		indexop = DO_IndexLines;
		break;

	case GL_LINE_LOOP:
		drawmode = GL_LINES;
		indexop = DO_IndexLineLoop;
		break;

	case GL_LINE_STRIP:
		drawmode = GL_LINES;
		indexop = DO_IndexLineStrip;
		break;

	case GL_TRIANGLES:
		drawmode = GL_TRIANGLES;
		indexop = DO_IndexTriangles;
		break;

	case GL_TRIANGLE_STRIP:
		drawmode = GL_TRIANGLES;
		indexop = DO_IndexTriangleStrip;
		break;

	case GL_TRIANGLE_FAN:
		drawmode = GL_TRIANGLES;
		indexop = DO_IndexTriangleFan;
		break;

	case GL_QUADS:
		drawmode = GL_TRIANGLES;
		indexop = DO_IndexQuads;
		break;

	case GL_QUAD_STRIP:
		drawmode = GL_TRIANGLES;
		indexop = DO_IndexQuadStrip;
		break;

	case GL_POLYGON:
		drawmode = GL_TRIANGLES;
		indexop = DO_IndexPolygon;
		break;
	}

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

	// add vertex-generation operations
	// this will set active components
	ConfigureDrawItems(aId, element, buffer, bake);

	// fill in the draw mode render state
	PackedRenderState &state = *new (&buffer[start]) PackedRenderState;
	state.mMode = unsigned char(drawmode);
	state.mNormal = sNormalActive;
	state.mColor = sColorActive;
	state.mTexCoord = sTexCoordActive;

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
// renderstate: pushattrib, popattrib, bindtexture, texenv, blendfunc

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

void ConfigureDrawItem(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
		//
		// TRANSFORM COMMANDS

	case 0x974c9474 /* "pushmatrix" */:
		{
			Expression::Append(buffer, DO_PushMatrix);
			ConfigureDrawItems(aId, element, buffer, bake);
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
			Expression::Append(buffer, DO_Vertex);
			Expression::Loader<DLPosition>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xe68b9c52 /* "normal" */:
		{
			sNormalActive = true;
			Expression::Append(buffer, DO_Normal);
			Expression::Loader<DLNormal>::ConfigureRoot(element, buffer, sNormalNames, sNormalDefault);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			sColorActive = true;
			Expression::Append(buffer, DO_Color);
			Expression::Loader<DLColor>::ConfigureRoot(element, buffer, sColorNames, sColorDefault);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			sTexCoordActive = true;
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
			ConfigurePrimitive(GL_POINTS, aId, element, buffer, bake);
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
			ConfigurePrimitive(GL_LINES, aId, element, buffer, bake);
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
			ConfigurePrimitive(GL_LINE_LOOP, aId, element, buffer, bake);
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
			ConfigurePrimitive(GL_LINE_STRIP, aId, element, buffer, bake);
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
			ConfigurePrimitive(GL_TRIANGLES, aId, element, buffer, bake);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			ConfigurePrimitive(GL_TRIANGLE_STRIP, aId, element, buffer, bake);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			ConfigurePrimitive(GL_TRIANGLE_FAN, aId, element, buffer, bake);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			ConfigurePrimitive(GL_QUADS, aId, element, buffer, bake);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			ConfigurePrimitive(GL_QUAD_STRIP, aId, element, buffer, bake);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			ConfigurePrimitive(GL_POLYGON, aId, element, buffer, bake);
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
				ConfigureDrawItems(aId, element, buffer, bake);
				Expression::Append(buffer, DO_PopAttribBake);
			}
			else
			{
				Expression::Append(buffer, DO_PushAttrib, mask);
				ConfigureDrawItems(aId, element, buffer, bake);
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

	case 0x059e3a91 /* "texenv" */:
		{
			// set blend mode
			GLenum blendmode;
			switch (Hash(element->Attribute("mode")))
			{
			default:
			case 0x818f75ae /* "modulate" */:	blendmode = GL_MODULATE; break;
			case 0xde15f6ae /* "decal" */:		blendmode = GL_DECAL; break;
			case 0x0bbc40d8 /* "blend" */:		blendmode = GL_BLEND; break;
			case 0xa13884c3 /* "replace" */:	blendmode = GL_REPLACE; break;
			}
			if (bake)
				Expression::Append(buffer, DO_TexEnvi, GL_TEXTURE_ENV_MODE, blendmode);
			else
				Expression::Append(buffer, DO_TexEnviBake, GL_TEXTURE_ENV_MODE, blendmode);
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

			if (bake)
				Expression::Append(buffer, DO_BlendFuncBake, srcfactor, dstfactor);
			else
				Expression::Append(buffer, DO_BlendFunc, srcfactor, dstfactor);
		}
		break;

		//
		// DRAWLIST COMMANDS

	case 0xc98b019b /* "drawlist" */:
		if (bake)
		{
			// inline the drawlist contents
			ConfigureDrawItems(aId, element, buffer, bake);
		}
		else
		{
			// create a generator drawlist
			Database::Key handle = Database::generatedrawlist.GetCount() + 1;
			std::vector<unsigned int> &generate = Database::generatedrawlist.Open(handle);
			assert(generate.size() == 0);

			// configure static data
			BeginStatic();
			ConfigureDrawItems(aId, element, generate, true);
			EndStatic(aId, buffer, generate);

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
			ConfigureDrawItems(aId, element, buffer, bake);
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
			ConfigureDrawItems(aId, element, buffer, bake);
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
			ConfigureDrawItems(aId, element, buffer, bake);
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

void ConfigureDrawItems(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureDrawItem(aId, child, buffer, bake);
	}
}

static void BeginStatic()
{
	// switch to static mode
	SetStaticActive(true);

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

	// clear active elements
	sNormalActive = false;
	sColorActive = false;
	sTexCoordActive = false;
}

static void EndStatic(unsigned int aId, std::vector<unsigned int> &buffer, const std::vector<unsigned int> &generate)
{
	// execute the drawlist
	BakeContext context(&buffer, &generate[0], generate.size(), 0.0f, aId);
	ExecuteDrawItems(context);

	// flush static
	FlushStatic(buffer);

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

void InitDrawlists(void)
{
	// bind function pointers
	glDrawRangeElements = static_cast<PFN_glDrawRangeElements>(glfwGetProcAddress("glDrawRangeElements"));
	glGenBuffers = static_cast<PFN_glGenBuffers>(glfwGetProcAddress("glGenBuffers"));
	glDeleteBuffers = static_cast<PFN_glDeleteBuffers>(glfwGetProcAddress("glDeleteBuffers"));
	glBindBuffer = static_cast<PFN_glBindBuffer>(glfwGetProcAddress("glBindBuffer"));
	glBufferData = static_cast<PFN_glBufferData>(glfwGetProcAddress("glBufferData"));
	glBufferSubData = static_cast<PFN_glBufferSubData>(glfwGetProcAddress("glBufferSubData"));
	glMapBuffer = static_cast<PFN_glMapBuffer>(glfwGetProcAddress("glMapBuffer"));
	glMapBufferRange = static_cast<PFN_glMapBufferRange>(glfwGetProcAddress("glMapBufferRange"));
	glUnmapBuffer = static_cast<PFN_glUnmapBuffer>(glfwGetProcAddress("glUnmapBuffer"));

	// set up dynamic vertex buffer
	{
		BufferState &state = sBuffer[BUFFER_DYNAMIC_VERTEX];
		state.mSize = 256 * 1024;
		glGenBuffers(1, &state.mHandle);
#ifdef DRAWLIST_DYNAMIC_BUFFER_RANGE
		glBindBuffer(GL_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ARRAY_BUFFER, state.mSize, NULL, GL_STREAM_DRAW);
#endif
		state.mData = NULL;
		state.mStart = 0;
		state.mEnd = 0;
	}

	// set up dynamic index buffer
	{
		BufferState &state = sBuffer[BUFFER_DYNAMIC_INDEX];
		state.mSize = 64 * 1024;
		glGenBuffers(1, &state.mHandle);
#ifdef DRAWLIST_DYNAMIC_BUFFER_RANGE
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, state.mSize, NULL, GL_STREAM_DRAW);
#endif
		state.mData = NULL;
		state.mStart = 0;
		state.mEnd = 0;
	}

	// set up static vertex buffer
	{
		BufferState &state = sBuffer[BUFFER_STATIC_VERTEX];
		state.mSize = 256 * 1024;
#ifdef DRAWLIST_STATIC_BUFFER
		glGenBuffers(1, &state.mHandle);
		glBindBuffer(GL_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ARRAY_BUFFER, state.mSize, NULL, GL_STATIC_DRAW);
#endif
		state.mData = malloc(state.mSize);
		state.mStart = 0;
		state.mEnd = 0;
	}

	// set up static index buffer
	{
		BufferState &state = sBuffer[BUFFER_STATIC_INDEX];
		state.mSize = 64 * 1024;
#ifdef DRAWLIST_STATIC_BUFFER
		glGenBuffers(1, &state.mHandle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, state.mSize, NULL, GL_STATIC_DRAW);
#endif
		state.mData = malloc(state.mSize);
		state.mStart = 0;
		state.mEnd = 0;
	}

	// vertex work buffer
	// this get copied to the array buffer on flush
	sVertexUsed = 0;
	sVertexCount = 0;
	sVertexBase = 0;

	// index work buffer
	// this gets copied to the element array buffer on flush
	sIndexCount = 0;

	// set to static mode for configure phase
	sStaticActive = true;
	sVertexBuffer = &sBuffer[BUFFER_STATIC_VERTEX];
	sIndexBuffer = &sBuffer[BUFFER_STATIC_INDEX];

#ifdef DRAWLIST_STATIC_BUFFER
	// bind static buffer
	glBindBuffer(GL_ARRAY_BUFFER, sVertexBuffer->mHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mHandle);
#else
	// bind dynamic buffer since that's the only choice
	glBindBuffer(GL_ARRAY_BUFFER, sBuffer[BUFFER_DYNAMIC_VERTEX].mHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sBuffer[BUFFER_DYNAMIC_INDEX].mHandle);
#endif

	// current vertex component values
	memcpy(&sNormal, sNormalDefault, sNormalWidth * sizeof(float));
#ifdef DRAWLIST_FLOAT_COLOR
	memcpy(&sColor, sColorDefault, sColorWidth * sizeof(float));
#else
	sColor[0] = sColor[1] = sColor[2] = sColor[3] = 255;
#endif
	memcpy(&sTexCoord, sTexCoordDefault, sTexCoordWidth * sizeof(float));

	// current drawing mode
	sDrawMode = GL_TRIANGLES;

	// active vertex components
	sNormalActive = false;
	sColorActive = false;
	sTexCoordActive = false;

	// initialize matrix stack
	StackInit();
}

// set static mode
static bool SetStaticActive(bool aStatic)
{
	if (sStaticActive == aStatic)
		return false;

	if (aStatic)
	{
		sVertexBuffer = &sBuffer[BUFFER_STATIC_VERTEX];
		sIndexBuffer = &sBuffer[BUFFER_STATIC_INDEX];
	}
	else
	{
		sVertexBuffer = &sBuffer[BUFFER_DYNAMIC_VERTEX];
		sIndexBuffer = &sBuffer[BUFFER_DYNAMIC_INDEX];
	}
#ifdef DRAWLIST_STATIC_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, sVertexBuffer->mHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mHandle);
#endif
	sStaticActive = aStatic;
	return true;
}


void CleanupDrawlists(void)
{
	// delete buffer objects
	for (int i = 0; i < BUFFER_COUNT; ++i)
	{
		glDeleteBuffers(1, &sBuffer[i].mHandle);
	}
}

void RebuildDrawlists(void)
{
#if 1

	// rebuild dynamic vertex buffer
	{
		BufferState &state = sBuffer[BUFFER_DYNAMIC_VERTEX];
		glGenBuffers(1, &state.mHandle);
#ifdef DRAWLIST_DYNAMIC_BUFFER_RANGE
		glBindBuffer(GL_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ARRAY_BUFFER, state.mSize, NULL, GL_STREAM_DRAW);
#endif
		state.mStart = 0;
		state.mEnd = 0;
	}

	// rebuild dynamic index buffer
	{
		BufferState &state = sBuffer[BUFFER_DYNAMIC_INDEX];
		glGenBuffers(1, &state.mHandle);
#ifdef DRAWLIST_DYNAMIC_BUFFER_RANGE
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, state.mSize, NULL, GL_STREAM_DRAW);
#endif
		state.mStart = 0;
		state.mEnd = 0;
	}

#ifdef DRAWLIST_STATIC_BUFFER
	// rebuild static vertex buffer
	{
		BufferState &state = sBuffer[BUFFER_STATIC_VERTEX];
		glGenBuffers(1, &state.mHandle);
		glBindBuffer(GL_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ARRAY_BUFFER, state.mSize, sVertexBuffer->mData, GL_STATIC_DRAW);
	}

	// rebuild static index buffer
	{
		BufferState &state = sBuffer[BUFFER_STATIC_INDEX];
		glGenBuffers(1, &state.mHandle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.mHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, state.mSize, sIndexBuffer->mData, GL_STATIC_DRAW);
	}
#endif

#ifdef DRAWLIST_STATIC_BUFFER
	// set to static mode for configure phase
	sStaticActive = true;
	sVertexBuffer = &sBuffer[BUFFER_STATIC_VERTEX];
	sIndexBuffer = &sBuffer[BUFFER_STATIC_INDEX];
	glBindBuffer(GL_ARRAY_BUFFER, sVertexBuffer->mHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sIndexBuffer->mHandle);
#endif

	// current vertex component values
	memcpy(&sNormal, sNormalDefault, sNormalWidth * sizeof(float));
#ifdef DRAWLIST_FLOAT_COLOR
	memcpy(&sColor, sColorDefault, sColorWidth * sizeof(float));
#else
	sColor[0] = sColor[1] = sColor[2] = 0;
	sColor[3] = 255;
#endif
	memcpy(&sTexCoord, sTexCoordDefault, sTexCoordWidth * sizeof(float));

#else

	CleanupDrawlists();
	InitDrawlists();

	// for each entry in the generator drawlist database...
	for (Database::Typed<std::vector<unsigned int> >::Iterator itor(&Database::generatedrawlist); itor.IsValid(); ++itor)
	{
		// get the generator
		Database::Key id = itor.GetKey();
		const std::vector<unsigned int> &generate = itor.GetValue();
		if (!generate.empty())
		{
			DebugPrint("rebuilding drawlist \"%s\"\n", Database::name.Get(id).c_str());

			// create a dummy drawlist
			std::vector<unsigned int> dummy;

			// rebuild the static buffers
			BeginStatic();
			EndStatic(id, dummy, generate);

			// TO DO: compare dummy against the original drawlist
			const std::vector<unsigned int> &drawlist = Database::drawlist.Get(id);
			if (!drawlist.empty())
			{
				if (drawlist.size() != dummy.size())
				{
					DebugPrint("size mismatch: before=%d after=%d\n", drawlist.size(), dummy.size());
				}
				else
				{
					for (size_t i = 0; i < drawlist.size(); ++i)
					{
						if (drawlist[i] != dummy[i])
						{
							DebugPrint("data mismatch at %d: before=%08x after=%08x\n", i, drawlist[i], dummy[i]);
							break;
						}
					}
				}
			}
		}
	}

#endif
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

void RenderFlush(void)
{
	// flush dynamic geometry
	// TO DO: only call this at the end of the frame
	FlushDynamic();
}