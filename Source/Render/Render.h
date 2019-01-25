#pragma once

#include "GL/glcorearb.h"

//
// OPENGL FUNCTIONS
//

// OpenGL 1.2
extern PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements;

// OpenGL 1.5
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLMAPBUFFERPROC glMapBuffer;
extern PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;

// OpenGL 2.0
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLISPROGRAMPROC glIsProgram;
extern PFNGLISSHADERPROC glIsShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM1FVPROC glUniform1fv;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
extern PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
extern PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
extern PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;
extern PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
extern PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;
extern PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
extern PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

// ARB vertex array object
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArray;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLISVERTEXARRAYPROC glIsVertexArray;

// Opengl 3.0
extern PFNGLGETSTRINGIPROC glGetStringi;


//
// RENDER SYSTEM FUNCTIONS
//

// bind OpenGL function pointers
extern void BindFunctionPointers(void);

// initalize rendering system
// (allocate persistent resources)
extern void InitRender(void);

// pre-reset rendering system
// (prepare for draw context reset)
extern void PreResetRender(void);

// post-reset rendering system
// (recreate after draw context reset)
extern void PostResetRender(void);

// clean up rendering system
// (free persistent resources)
extern void CleanupRender(void);


//
// SHADER PROGRAM FUNCTIONS
//

// create a vertex shader
extern GLuint CreateVertexShader(const char * aShaderCode);

// create a fragment shader
extern GLuint CreateFragmentShader(const char * aShaderCode);

// create a shader program
extern GLuint CreateProgram(GLuint aVertexShaderId, GLuint aFragmentShaderId);

// link a shader program
extern GLint LinkProgram(GLuint aProgramId);

// delete a shader
extern void DeleteShader(GLuint aShaderId);

// delete a shader program
extern void DeleteProgram(GLuint aProgramId);

// use a shader program
// forces a render flush if different from the current program
// returns true if it changed the program
extern bool UseProgram(GLuint aProgram);

// get shader program currently in use
extern GLuint GetProgramInUse(void);

//
// BUFFER OBJECTS
//

struct BufferObject
{
	// allocated size
	GLuint mSize;

	// buffer handle
	GLuint mHandle;

	// buffer target
	// GL_ARRAY_BUFFER
	// GL_ELEMENT_ARRAY_BUFFER
	GLenum mTarget;

	// buffer usage
	// GL_STREAM_DRAW
	// GL_STATIC_DRAW
	// GL_DYNAMIC_DRAW
	GLenum mUsage;

	// persist data
	void *mPersist;

	// working range
	GLuint mStart;
	GLuint mEnd;
};

extern void BufferInit(BufferObject &aBuffer, GLenum aTarget, GLenum aUsage);
extern void BufferGen(BufferObject &aBuffer);
extern void BufferBind(BufferObject &aBuffer);
extern void BufferSetData(BufferObject &aBuffer, GLuint aSize, void *aData);
extern void BufferAppendData(BufferObject &aBuffer, GLuint aSize, void *aData);
extern void BufferCleanup(BufferObject &aBuffer);

extern BufferObject &GetBoundVertexBuffer(void);
extern BufferObject &GetBoundIndexBuffer(void);
extern BufferObject &GetDynamicVertexBuffer(void);
extern BufferObject &GetDynamicIndexBuffer(void);


//
// UNIFORM FUNCTIONS
//

extern void SetUniformFloat(GLint aIndex, const float aValue);
extern void SetUniformVector4(GLint aIndex, const float aValue[]);
extern void SetUniformMatrix4(GLint aIndex, const float aValue[]);


//
// ATTRIB FUNCTIONS
//

// preset attribute indices
// TO DO: come up with an alternative
enum AttribIndex
{
	ATTRIB_INDEX_POSITION = 0,
	ATTRIB_INDEX_COLOR = 1,
	ATTRIB_INDEX_TEXCOORD = 2,
	ATTRIB_INDEX_COUNT
};

extern void SetAttribCount(GLint aCount);
extern void SetAttribFormat(GLint aIndex, GLuint aWidth, GLenum aType);
extern void SetAttribConstant(GLint aIndex, __m128 aValue);
extern void SetAttribBuffer(GLint aIndex, BufferObject &aBuffer, GLuint aStride, GLuint aOffset);


//
// INDEX FUNCTIONS
//

extern void SetIndexBuffer(BufferObject &aBuffer);


//
// DRAW FUNCTIONS
//

extern void BeginScene(void);
extern void EndScene(void);
extern bool IsDynamicActive(void);
extern void SetWorkFormat(GLuint aFormat);
extern GLuint GetWorkFormat(void);
extern void SetDrawMode(GLenum aDrawMode);
extern void SetAttribValue(GLint aIndex, __m128 aValue);
extern void AddVertex(void);
extern void *AllocVertices(GLuint aCount);
extern void *AllocIndices(GLuint aCount);
extern void AddVertices(GLuint aCount, const void *aData);
extern void IndexLines(GLuint aStart, GLuint aCount);
extern void IndexLineLoop(GLuint aStart, GLuint aCount);
extern void IndexLineStrip(GLuint aStart, GLuint aCount);
extern void IndexTriangles(GLuint aStart, GLuint aCount);
extern void IndexTriangleStrip(GLuint aStart, GLuint aCount);
extern void IndexTriangleFan(GLuint aStart, GLuint aCount);
extern void IndexQuads(GLuint aStart, GLuint aCount);
extern void IndexQuadStrip(GLuint aStart, GLuint aCount);
extern void IndexPolygon(GLuint aStart, GLuint aCount);
extern GLuint GetVertexCount(void);
extern GLuint GetIndexCount(void);
extern void FlushDynamic(void);

// draw non-indexed primitive from currently bound buffers
extern void DrawArrays(GLenum aDrawMode, /*GLuint aVertexFirst,*/ GLuint aVertexCount);

// draw indexed primitive from currently bound buffers
extern void DrawElements(GLenum aDrawMode, /*GLuint aVertexFirst,*/ GLuint aVertexCount, GLuint aIndexCount, GLuint aIndexOffset);

// bind a texture
extern void BindTexture(GLuint texture);

// copy sub-image to the current texture
extern void CopyTexSubImage(GLint aLevel, GLint aXOffset, GLint aYOffset, GLint aX, GLint aY, GLsizei aWidth, GLsizei aHeight);

// clear the screen
extern void ClearFrame(void);

// clear color
extern void SetClearColor(const Color4 &aColor);
extern const Color4 &GetClearColor(void);

// set fog properties
extern void SetFogEnabled(bool aEnable);
extern bool GetFogEnabled(void);
extern void SetFogColor(const Color4 &aColor);
extern const Color4 &GetFogColor(void);
extern void SetFogStart(float aStart);
extern void SetFogEnd(float aEnd);
extern float GetFogStart(void);
extern float GetFogEnd(void);
