#include "StdAfx.h"
#include "Render.h"
#include "Drawlist.h"
#include "Font.h"
#include "Expression.h"
#include "MatrixStack.h"

// queue rendering operations?
// (disabling reduces performance but makes debugging easier)
#define RENDER_USE_QUEUE


//
// OPENGL FUNCTIONS
//

// OpenGL 1.3
PFNGLACTIVETEXTUREPROC glActiveTexture;

// OpenGL 1.5
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;

// OpenGL 2.0
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLDRAWBUFFERSPROC glDrawBuffers;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDETACHSHADERPROC glDetachShader;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLISPROGRAMPROC glIsProgram;
PFNGLISSHADERPROC glIsShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;
PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;
PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

// ARB vertex array object
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLISVERTEXARRAYPROC glIsVertexArray;

// OpenGL 3.0
PFNGLGETSTRINGIPROC glGetStringi;

//
// BUFFER OBJECTS
//

// dynamic buffers
static BufferObject sDynamicVertexBuffer;
static BufferObject sDynamicIndexBuffer;

// vertex array object
static GLuint sVertexArray;

//
// DRAW STATE
// 

#ifdef RENDER_USE_QUEUE
// render queue
std::vector<unsigned int> sRenderQueue;
#endif

// active shader program
GLuint sProgram;

// active draw mode
GLenum sDrawMode;

// active texture
GLuint sTexture;

// currently bound buffers
BufferObject *sVertexBuffer;
BufferObject *sIndexBuffer;

// vertex attributes
const int RENDER_MAX_ATTRIB = 16;
int sAttribCount;
__m128 sAttribValue[RENDER_MAX_ATTRIB];
GLuint sAttribBuffer[RENDER_MAX_ATTRIB];
GLuint sAttribWidth[RENDER_MAX_ATTRIB];
GLenum sAttribType[RENDER_MAX_ATTRIB];
GLuint sAttribSize[RENDER_MAX_ATTRIB];
GLuint sAttribStride[RENDER_MAX_ATTRIB];
GLuint sAttribOffset[RENDER_MAX_ATTRIB];
GLuint sAttribDisplace[RENDER_MAX_ATTRIB];

// packed vertex data
GLubyte sVertexPacked[RENDER_MAX_ATTRIB*16];

// vertex work buffer format
GLuint sVertexWorkFormat;
GLuint sVertexWorkSize;

// vertex pool
// TO DO: move this to drawlist?
static float sVertexPool[64 * 1024];
GLuint sVertexLimit = 64 * 1024;

// index pool
// TO DO: move this to drawlist?
static unsigned short sIndexPool[16 * 1024];
GLuint sIndexLimit = 16 * 1024;

// vertex work buffer
float *sVertexWork = sVertexPool;
GLuint sVertexUsed;
GLuint sVertexCount;
GLuint sVertexBase;

// index work buffer
unsigned short *sIndexWork = sIndexPool;
GLuint sIndexCount;

// clear color
Color4 sClearColor;

// fog properties
bool sFogEnabled;
Color4 sFogColor;
float sFogStart;
float sFogEnd;


//
// RENDER SYSTEM FUNCTIONS
//

// bind function pointers
void BindFunctionPointers(void)
{
	// bind 1.3 function pointers
	glActiveTexture = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(glfwGetProcAddress("glActiveTexture"));

	// bind 1.5 function pointers
	glGenBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(glfwGetProcAddress("glGenBuffers"));
	glDeleteBuffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(glfwGetProcAddress("glDeleteBuffers"));
	glBindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(glfwGetProcAddress("glBindBuffer"));
	glBufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(glfwGetProcAddress("glBufferData"));
	glBufferSubData = reinterpret_cast<PFNGLBUFFERSUBDATAPROC>(glfwGetProcAddress("glBufferSubData"));
	glMapBuffer = reinterpret_cast<PFNGLMAPBUFFERPROC>(glfwGetProcAddress("glMapBuffer"));
	glMapBufferRange = reinterpret_cast<PFNGLMAPBUFFERRANGEPROC>(glfwGetProcAddress("glMapBufferRange"));
	glUnmapBuffer = reinterpret_cast<PFNGLUNMAPBUFFERPROC>(glfwGetProcAddress("glUnmapBuffer"));

	// bind 2.0 function pointers
	glAttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(glfwGetProcAddress("glAttachShader"));
	glDrawBuffers = reinterpret_cast<PFNGLDRAWBUFFERSPROC>(glfwGetProcAddress("glDrawBuffers"));
	glBindAttribLocation = reinterpret_cast<PFNGLBINDATTRIBLOCATIONPROC>(glfwGetProcAddress("glBindAttribLocation"));
	glCompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(glfwGetProcAddress("glCompileShader"));
	glCreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(glfwGetProcAddress("glCreateProgram"));
	glCreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(glfwGetProcAddress("glCreateShader"));
	glDeleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(glfwGetProcAddress("glDeleteProgram"));
	glDeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(glfwGetProcAddress("glDeleteShader"));
	glDetachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(glfwGetProcAddress("glDetachShader"));
	glDisableVertexAttribArray = reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(glfwGetProcAddress("glDisableVertexAttribArray"));
	glEnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(glfwGetProcAddress("glEnableVertexAttribArray"));
	glGetAttribLocation = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(glfwGetProcAddress("glGetAttribLocation"));
	glGetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(glfwGetProcAddress("glGetProgramiv"));
	glGetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(glfwGetProcAddress("glGetProgramInfoLog"));
	glGetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(glfwGetProcAddress("glGetShaderiv"));
	glGetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(glfwGetProcAddress("glGetShaderInfoLog"));
	glGetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(glfwGetProcAddress("glGetUniformLocation"));
	glIsProgram = reinterpret_cast<PFNGLISPROGRAMPROC>(glfwGetProcAddress("glIsProgram"));
	glIsShader = reinterpret_cast<PFNGLISSHADERPROC>(glfwGetProcAddress("glIsShader"));
	glLinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(glfwGetProcAddress("glLinkProgram"));
	glShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(glfwGetProcAddress("glShaderSource"));
	glUseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(glfwGetProcAddress("glUseProgram"));
	glUniform1f = reinterpret_cast<PFNGLUNIFORM1FPROC>(glfwGetProcAddress("glUniform1f"));
	glUniform2f = reinterpret_cast<PFNGLUNIFORM2FPROC>(glfwGetProcAddress("glUniform2f"));
	glUniform3f = reinterpret_cast<PFNGLUNIFORM3FPROC>(glfwGetProcAddress("glUniform3f"));
	glUniform4f = reinterpret_cast<PFNGLUNIFORM4FPROC>(glfwGetProcAddress("glUniform4f"));
	glUniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(glfwGetProcAddress("glUniform1i"));
	glUniform1fv = reinterpret_cast<PFNGLUNIFORM1FVPROC>(glfwGetProcAddress("glUniform1fv"));
	glUniform2fv = reinterpret_cast<PFNGLUNIFORM2FVPROC>(glfwGetProcAddress("glUniform2fv"));
	glUniform3fv = reinterpret_cast<PFNGLUNIFORM3FVPROC>(glfwGetProcAddress("glUniform3fv"));
	glUniform4fv = reinterpret_cast<PFNGLUNIFORM4FVPROC>(glfwGetProcAddress("glUniform4fv"));
	glUniformMatrix4fv = reinterpret_cast<PFNGLUNIFORMMATRIX4FVPROC>(glfwGetProcAddress("glUniformMatrix4fv"));
	glValidateProgram = reinterpret_cast<PFNGLVALIDATEPROGRAMPROC>(glfwGetProcAddress("glValidateProgram"));
	glVertexAttrib1f = reinterpret_cast<PFNGLVERTEXATTRIB1FPROC>(glfwGetProcAddress("glVertexAttrib1f"));
	glVertexAttrib1fv = reinterpret_cast<PFNGLVERTEXATTRIB1FVPROC>(glfwGetProcAddress("glVertexAttrib1fv"));
	glVertexAttrib2f = reinterpret_cast<PFNGLVERTEXATTRIB2FPROC>(glfwGetProcAddress("glVertexAttrib2f"));
	glVertexAttrib2fv = reinterpret_cast<PFNGLVERTEXATTRIB2FVPROC>(glfwGetProcAddress("glVertexAttrib2fv"));
	glVertexAttrib3f = reinterpret_cast<PFNGLVERTEXATTRIB3FPROC>(glfwGetProcAddress("glVertexAttrib3f"));
	glVertexAttrib3fv = reinterpret_cast<PFNGLVERTEXATTRIB3FVPROC>(glfwGetProcAddress("glVertexAttrib3fv"));
	glVertexAttrib4f = reinterpret_cast<PFNGLVERTEXATTRIB4FPROC>(glfwGetProcAddress("glVertexAttrib4f"));
	glVertexAttrib4fv = reinterpret_cast<PFNGLVERTEXATTRIB4FVPROC>(glfwGetProcAddress("glVertexAttrib4fv"));
	glVertexAttrib4Nubv = reinterpret_cast<PFNGLVERTEXATTRIB4NUBVPROC>(glfwGetProcAddress("glVertexAttrib4Nubv"));
	glVertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(glfwGetProcAddress("glVertexAttribPointer"));

	// ARB vertex array object
	glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(glfwGetProcAddress("glBindVertexArray"));
	glDeleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(glfwGetProcAddress("glDeleteVertexArrays"));
	glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(glfwGetProcAddress("glGenVertexArrays"));
	glIsVertexArray = reinterpret_cast<PFNGLISVERTEXARRAYPROC>(glfwGetProcAddress("glIsVertexArray"));

	// bind 3.0 function pointers
	glGetStringi = reinterpret_cast<PFNGLGETSTRINGIPROC>(glfwGetProcAddress("glGetStringi"));
}

// initialize work buffer
static void InitWorkBuffer(void)
{
	// vertex work buffer
	sVertexUsed = 0;
	sVertexCount = 0;
	sVertexBase = 0;

	// index work buffer
	sIndexCount = 0;
}

// reset attributes
static void InitAttribs(void)
{
	memset(sAttribBuffer, 0, sAttribCount * sizeof(sAttribBuffer[0]));
	memset(sAttribWidth, 0, sAttribCount * sizeof(sAttribWidth[0]));
	memset(sAttribType, 0, sAttribCount * sizeof(sAttribType[0]));
	memset(sAttribSize, 0, sAttribCount * sizeof(sAttribSize[0]));
	memset(sAttribStride, 0, sAttribCount * sizeof(sAttribStride[0]));
	memset(sAttribOffset, 0, sAttribCount * sizeof(sAttribOffset[0]));
	memset(sAttribDisplace, 0, sAttribCount * sizeof(sAttribDisplace[0]));
	sAttribCount = 0;
}

// init vertex format
static void InitVertexFormat(void)
{
	sVertexWorkFormat = 0;
	sVertexWorkSize = 0;
}

// clear render state
static void ClearRenderState(void)
{
	// clear program
	sProgram = 0;

	// set to triangle mode
	sDrawMode = GL_TRIANGLES;

	// no texture
	sTexture = 0;

	// clear active buffer
	sVertexBuffer = NULL;
	sIndexBuffer = NULL;

	// initialize attribs
	InitAttribs();

	// init vertex format
	InitVertexFormat();

	// initialize work buffer
	InitWorkBuffer();
}

// initialize rendering
void InitRender(void)
{
	// set up dynamic vertex buffer
	BufferInit(sDynamicVertexBuffer, GL_ARRAY_BUFFER, GL_STREAM_DRAW);
	BufferGen(sDynamicVertexBuffer);
	BufferSetData(sDynamicVertexBuffer, 512 * 1024, NULL);

	// set up dynamic index buffer
	BufferInit(sDynamicIndexBuffer, GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW);
	BufferGen(sDynamicIndexBuffer);
	BufferSetData(sDynamicIndexBuffer, 128 * 1024, NULL);

	// set up shared vertex array
	glGenVertexArrays(1, &sVertexArray);
	glBindVertexArray(sVertexArray);

	// clear render state
	ClearRenderState();

	// disable attribute arrays
	for (int i = 0; i < RENDER_MAX_ATTRIB; ++i)
		glDisableVertexAttribArray(i);
#ifdef SUPPORT_FIXED_FUNCTION
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

#ifdef RENDER_USE_QUEUE
	// initialize render queue
	sRenderQueue.reserve(16 * 1024);
#endif

	// clear color
	sClearColor = Color4(0, 0, 0, 1);

	// fog
	sFogEnabled = false;
	sFogColor = Color4(0, 0, 0, 1);
	sFogStart = FLT_MAX;
	sFogEnd = FLT_MAX;
}

// pre-reset rendering system
void PreResetRender(void)
{
#ifdef RENDER_USE_QUEUE
	sRenderQueue.clear();
#endif
	EndScene();

	// release program so it can be freed
	sProgram = 0;
	glUseProgram(0);

	// release buffers
	BufferCleanup(sDynamicVertexBuffer);
	BufferCleanup(sDynamicIndexBuffer);
}

// post-reset rendering system 
void PostResetRender(void)
{
	// rebuild dynamic vertex buffer
	BufferGen(sDynamicVertexBuffer);
	BufferSetData(sDynamicVertexBuffer, sDynamicVertexBuffer.mSize, NULL);
	sDynamicVertexBuffer.mStart = 0;
	sDynamicVertexBuffer.mEnd = 0;

	// rebuild dynamic index buffer
	BufferGen(sDynamicIndexBuffer);
	BufferSetData(sDynamicIndexBuffer, sDynamicIndexBuffer.mSize, NULL);
	sDynamicIndexBuffer.mStart = 0;
	sDynamicIndexBuffer.mEnd = 0;

	// clear render state
	ClearRenderState();

	// restore colors
	SetClearColor(sClearColor);
	SetFogEnabled(sFogEnabled);
	SetFogColor(sFogColor);
	SetFogStart(sFogStart);
	SetFogEnd(sFogEnd);
}

// clean up rendering
void CleanupRender(void)
{
#ifdef RENDER_USE_QUEUE
	// clear render queue
	sRenderQueue.clear();
#endif

	// clear program
	sProgram = 0;
	glUseProgram(0);

	// clear active buffer
	sVertexBuffer = NULL;
	sIndexBuffer = NULL;

	BufferCleanup(sDynamicVertexBuffer);
	BufferCleanup(sDynamicIndexBuffer);
}


#ifdef RENDER_USE_QUEUE
//
// RENDER QUEUE OPERATIONS
//

// use a shader program
// 0 means fixed-function pipeline
void RQ_UseProgram(Expression::Context &aContext)
{
	GLuint program(Expression::Read<GLuint>(aContext));
	if (sProgram && !program)
	{
		// switch off vertex attribute arrays
		for (int i = 0; i < sAttribCount; ++i)
		{
			if (sAttribBuffer[i])
			{
				glDisableVertexAttribArray(i);
			}
		}
	}
	glUseProgram(program);
	sProgram = program;
}

// set uniform float (shader)
void RQ_UniformFloat(Expression::Context &aContext)
{
	assert(sProgram != 0);
	GLint index(Expression::Read<GLint>(aContext));
	GLfloat value(Expression::Read<GLfloat>(aContext));
	glUniform1f(index, value);
}

// set uniform vector (shader)
void RQ_UniformVector4(Expression::Context &aContext)
{
	assert(sProgram != 0);
	GLint index(Expression::Read<GLint>(aContext));
	glUniform4fv(index, 1, reinterpret_cast<const float *>(aContext.mStream));
	aContext.mStream += 4;
}

// set uniform matrix (shader)
struct M { float m[16]; };
void RQ_UniformMatrix4(Expression::Context &aContext)
{
	assert(sProgram != 0);
	GLint index(Expression::Read<GLint>(aContext));
	glUniformMatrix4fv(index, 1, GL_FALSE, reinterpret_cast<const float *>(aContext.mStream));
	aContext.mStream += 16;
}

// load a matrix (fixed-function)
void RQ_LoadMatrix4(Expression::Context &aContext)
{
	GLenum mode(Expression::Read<GLint>(aContext));
	if (mode == GL_PROJECTION)
		glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(reinterpret_cast<const float *>(aContext.mStream));
	if (mode == GL_PROJECTION)
		glMatrixMode(GL_MODELVIEW);
	aContext.mStream += 16;
}

// disable an attribute
void RQ_AttribDisable(Expression::Context &aContext)
{
	assert(sProgram != 0);
	GLint index(Expression::Read<GLint>(aContext));
	glDisableVertexAttribArray(index);
}

// set an attribute constant (shader)
void RQ_AttribConst(Expression::Context &aContext)
{
	assert(sProgram != 0);
	GLint index(Expression::Read<GLint>(aContext));
	__m128 value(Expression::Read<__m128>(aContext));
	glDisableVertexAttribArray(index);
	glVertexAttrib4fv(index, value.m128_f32);
}

// set normal constant (fixed-function)
void RQ_NormalConst(Expression::Context &aContext)
{
	assert(sProgram == 0);
	__m128 value(Expression::Read<__m128>(aContext));
	glDisableClientState(GL_NORMAL_ARRAY);
	glNormal3fv(value.m128_f32);
}

// set color constant (fixed-function)
void RQ_ColorConst(Expression::Context &aContext)
{
	assert(sProgram == 0);
	__m128 value(Expression::Read<__m128>(aContext));
	glDisableClientState(GL_COLOR_ARRAY);
	glColor4fv(value.m128_f32);
}

// set texture coordinate constant (fixed-function)
void RQ_TexCoordConst(Expression::Context &aContext)
{
	assert(sProgram == 0);
	__m128 value(Expression::Read<__m128>(aContext));
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoord2fv(value.m128_f32);
}

// bind vertex buffer
void RQ_BindVertexBuffer(Expression::Context &aContext)
{
	GLuint buffer(Expression::Read<GLuint>(aContext));
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
}

// bind index buffer
void RQ_BindIndexBuffer(Expression::Context &aContext)
{
	GLuint buffer(Expression::Read<GLuint>(aContext));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
}

// set attribute array (shader)
struct AttribArrayDesc
{
	GLubyte index;
	GLubyte width;
	GLubyte type;
	GLubyte stride;
	GLuint offset;
};
void RQ_AttribArray(Expression::Context &aContext)
{
	assert(sProgram != 0);
	AttribArrayDesc desc(Expression::Read<AttribArrayDesc>(aContext));
	glEnableVertexAttribArray(desc.index);
	glVertexAttribPointer(desc.index, desc.width, desc.type + GL_BYTE, GL_TRUE, desc.stride, reinterpret_cast<const GLvoid *>(desc.offset));
}

// set vertex array (fixed function)
void RQ_VertexArray(Expression::Context &aContext)
{
	assert(sProgram == 0);
	AttribArrayDesc desc(Expression::Read<AttribArrayDesc>(aContext));
	glVertexPointer(desc.width, desc.type + GL_BYTE, desc.stride, reinterpret_cast<const GLvoid *>(desc.offset));
}

// set normal array (fixed function)
void RQ_NormalArray(Expression::Context &aContext)
{
	assert(sProgram == 0);
	AttribArrayDesc desc(Expression::Read<AttribArrayDesc>(aContext));
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(desc.type + GL_BYTE, desc.stride, reinterpret_cast<const GLvoid *>(desc.offset));
}

// set color array (fixed-function)
void RQ_ColorArray(Expression::Context &aContext)
{
	assert(sProgram == 0);
	AttribArrayDesc desc(Expression::Read<AttribArrayDesc>(aContext));
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(desc.width, desc.type + GL_BYTE, desc.stride, reinterpret_cast<const GLvoid *>(desc.offset));
}

// set color array (fixed-function)
void RQ_TexCoordArray(Expression::Context &aContext)
{
	assert(sProgram == 0);
	AttribArrayDesc desc(Expression::Read<AttribArrayDesc>(aContext));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(desc.width, desc.type + GL_BYTE, desc.stride, reinterpret_cast<const GLvoid *>(desc.offset));
}

// bind texture
void RQ_BindTexture(Expression::Context &aContext)
{
	GLuint texture(Expression::Read<GLuint>(aContext));
#if 0
	if (texture)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
#endif
	glBindTexture(GL_TEXTURE_2D, texture);
}

// copy sub-image
void RQ_CopyTexSubImage(Expression::Context &aContext)
{
	GLint level(Expression::Read<GLint>(aContext));
	GLint xoffset(Expression::Read<GLint>(aContext));
	GLint yoffset(Expression::Read<GLint>(aContext));
	GLint x(Expression::Read<GLint>(aContext));
	GLint y(Expression::Read<GLint>(aContext));
	GLsizei width(Expression::Read<GLsizei>(aContext));
	GLsizei height(Expression::Read<GLsizei>(aContext));
	glCopyTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, x, y, width, height);
}

// draw non-indexed primitive
struct DrawArraysDesc
{
	GLubyte mode;
	GLushort count;
};
void RQ_DrawArrays(Expression::Context &aContext)
{
	DrawArraysDesc desc(Expression::Read<DrawArraysDesc>(aContext));
	glDrawArrays(desc.mode, 0, desc.count);
}

// draw indexed primitive
struct DrawElementsDesc
{
	GLubyte mode;
	GLushort count;
	GLuint offset;
};
void RQ_DrawElements(Expression::Context &aContext)
{
	DrawElementsDesc desc(Expression::Read<DrawElementsDesc>(aContext));
	glDrawElements(desc.mode, desc.count, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid *>(desc.offset));
}

void RQ_ClearFrame(Expression::Context &aContext)
{
	// clear the screen
	glClear(
		GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_TEST
		| GL_DEPTH_BUFFER_BIT
#endif
		);
}
#endif


//
// SHADER PROGRAM FUNCTIONS
//

// check shader status
static GLint CheckShader(GLuint aHandle, const char *aMessage)
{
	GLint status;
	glGetShaderiv(aHandle, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		GLchar log[1024];
		GLsizei len;
		glGetShaderInfoLog(aHandle, 1024, &len, log);
		DebugPrint("%s\n%s\n", aMessage, log);
	}
	return status;
}

// create a vertex shader
GLuint CreateVertexShader(const char * aShaderCode)
{
	// compile vertex shader
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderId, 1, &aShaderCode, NULL);
	glCompileShader(vertexShaderId);
	CheckShader(vertexShaderId, "Error compiling vertex shader:");
	return vertexShaderId;
}

// create a fragment shader
GLuint CreateFragmentShader(const char * aShaderCode)
{
	// compile fragment shader
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderId, 1, &aShaderCode, NULL);
	glCompileShader(fragmentShaderId);
	CheckShader(fragmentShaderId, "Error compiling fragment shader:");
	return fragmentShaderId;
}

// check program status
static GLint CheckProgram(GLuint aHandle, const char *aMessage)
{
	GLint status;
	glGetProgramiv(aHandle, GL_LINK_STATUS, &status);
	if (!status)
	{
		GLchar log[1024];
		GLsizei len;
		glGetProgramInfoLog(aHandle, 1024, &len, log);
		DebugPrint("%s\n%s\n", aMessage, log);
	}
	return status;
}

// create a shader program
GLuint CreateProgram(GLuint aVertexShaderId, GLuint aFragmentShaderId)
{
	// create shader program
	GLuint program = glCreateProgram();
	glAttachShader(program, aVertexShaderId);
	glAttachShader(program, aFragmentShaderId);
	glLinkProgram(program);
	CheckProgram(program, "Error linking shader program:");

	return program;
}

// delete a shader
void DeleteShader(GLuint aShaderId)
{
	if (glIsShader(aShaderId))
		glDeleteShader(aShaderId);
}

// delete a shader program
void DeleteProgram(GLuint aProgramId)
{
	if (glIsProgram(aProgramId))
		glDeleteProgram(aProgramId);
}

// use shader program
bool UseProgram(GLuint aProgram)
{
	if (sProgram == aProgram)
		return false;

	FlushDynamic();

#ifndef RENDER_USE_QUEUE
	if (sProgram && !aProgram)
	{
		// switch off vertex attribute arrays
		for (int i = 0; i < sAttribCount; ++i)
		{
			if (sAttribBuffer[i])
			{
				glDisableVertexAttribArray(i);
			}
		}
	}
#endif

	// initialize attribs
	InitAttribs();

	// init vertex format
	InitVertexFormat();

#ifdef RENDER_USE_QUEUE
	// queue use program command
	Expression::Append(sRenderQueue, RQ_UseProgram, aProgram);
#else
	glUseProgram(aProgram);
#endif
	sProgram = aProgram;

	return true;
}

// get shader program currently in use
GLuint GetProgramInUse(void)
{
	return sProgram;
}


//
// BUFFER FUNCTIONS
//

// initialize a buffer
void BufferInit(BufferObject &aBuffer, GLenum aTarget, GLenum aUsage)
{
	aBuffer.mSize = 0;
	aBuffer.mHandle = 0;
	aBuffer.mTarget = aTarget;
	aBuffer.mUsage = aUsage;
	aBuffer.mPersist = NULL;
	aBuffer.mStart = 0;
	aBuffer.mEnd = 0;
}

// generate buffer
void BufferGen(BufferObject &aBuffer)
{
	assert(aBuffer.mHandle == 0);
	glGenBuffers(1, &aBuffer.mHandle);
}

// bind buffer
void BufferBind(BufferObject &aBuffer)
{
	glBindBuffer(aBuffer.mTarget, aBuffer.mHandle);
}

// set buffer data
void BufferSetData(BufferObject &aBuffer, GLuint aSize, void *aData)
{
	BufferBind(aBuffer);
	aBuffer.mSize = aSize;
	glBufferData(aBuffer.mTarget, aSize, aData, aBuffer.mUsage);
}

// cleanup buffer
void BufferCleanup(BufferObject &aBuffer)
{
	glDeleteBuffers(1, &aBuffer.mHandle);
	aBuffer.mHandle = 0;
}

// append data to buffer
void BufferAppendData(BufferObject &aBuffer, GLuint aSize, void *aData)
{
	// if the buffer is full...
	if (aBuffer.mEnd + aSize > aBuffer.mSize)
	{
		if (aBuffer.mUsage == GL_STATIC_DRAW)
		{
			// buffer is full; cannot proceed
			DebugPrint("Static %s buffer %d is full",
				aBuffer.mTarget == GL_ARRAY_BUFFER ? "vertex" : "index",
				aBuffer.mHandle);
			return;
		}
		else
		{
			// rewind to the beginning
			aBuffer.mStart = 0;
			aBuffer.mEnd = 0;
		}
	}

	// bind the buffer
	BufferBind(aBuffer);

	// append data to the buffer
	glBufferSubData(aBuffer.mTarget, aBuffer.mEnd, aSize, aData);

	// if the buffer has persistent storage...
	if (aBuffer.mPersist)
	{
		// append data to persistent storage
		memcpy(reinterpret_cast<GLubyte *>(aBuffer.mPersist) + aBuffer.mEnd, aData, aSize);
	}

	// advance the end offset
	aBuffer.mEnd += aSize;
}

// get the currently-bound vertex buffer
BufferObject &GetBoundVertexBuffer(void)
{
	return *sVertexBuffer;
}

// get the currently-bound index buffer
BufferObject &GetBoundIndexBuffer(void)
{
	return *sIndexBuffer;
}

// get the shared dynamic vertex buffer
BufferObject &GetDynamicVertexBuffer(void)
{
	return sDynamicVertexBuffer;
}

// get the shared dynamic index buffer
BufferObject &GetDynamicIndexBuffer(void)
{
	return sDynamicIndexBuffer;
}


//
// UNIFORM FUNCTIONS
//

// set uniform float
extern void SetUniformFloat(GLint aIndex, const float aValue)
{
#ifdef RENDER_USE_QUEUE
	Expression::Append(sRenderQueue, RQ_UniformFloat, aIndex, aValue);
#else
	glUniform1f(aIndex, aValue);
#endif
}

// set uniform vector
extern void SetUniformVector4(GLint aIndex, const float aValue[])
{
#ifdef RENDER_USE_QUEUE
	Expression::Append(sRenderQueue, RQ_UniformVector4, aIndex);
	memcpy(Expression::Alloc(sRenderQueue, 4 * sizeof(float)), aValue, 4 * sizeof(float));
#else
	glUniform4fv(aIndex, 1, aValue);
#endif
}

// set uniform matrix
static void SetUniformMatrix4Internal(GLint aIndex, const float aValue[])
{
#ifdef RENDER_USE_QUEUE
	// queue matrix command
	Expression::Append(sRenderQueue, 
		(aIndex == GL_PROJECTION || aIndex == GL_MODELVIEW) ? RQ_LoadMatrix4 : RQ_UniformMatrix4,
		aIndex);
	memcpy(Expression::Alloc(sRenderQueue, 16 * sizeof(float)), aValue, 16 * sizeof(float));
#else
	if (aIndex == GL_PROJECTION)
	{
		// fixed-function projection
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(aValue);
		glMatrixMode(GL_MODELVIEW);
	}
	else if (aIndex == GL_MODELVIEW)
	{
		// fixed-function modelview
		glLoadMatrixf(aValue);
	}
	else
	{
		// set uniform matrix
		glUniformMatrix4fv(aIndex, 1, GL_FALSE, aValue);
	}
#endif
}
void SetUniformMatrix4(GLint aIndex, const float aValue[])
{
	// flush pending geometry
	FlushDynamic();

	SetUniformMatrix4Internal(aIndex, aValue);
}


//
// ATTRIB FUNCTIONS
//

// fixed-function attributes
// TO DO: use indices based on GL_*_ARRAY enum
static const GLenum sFixedAttrib[] =
{
	GL_VERTEX_ARRAY,
	GL_NORMAL_ARRAY,
	GL_COLOR_ARRAY,
	GL_TEXTURE_COORD_ARRAY,
};

#ifdef RENDER_USE_QUEUE

// TO DO: use indices based on GL_*_ARRAY enum
typedef void (*FixedAttribOp)(Expression::Context &aContext);

// fixed-function attribute array functions
// TO DO: use indices based on GL_*_ARRAY enum
const FixedAttribOp sFixedAttribArray[] =
{
	RQ_VertexArray,
	RQ_NormalArray,
	RQ_ColorArray,
	RQ_TexCoordArray,
};

// fixed-function attribute constant functions
// TO DO: use indices based on GL_*_ARRAY enum
const FixedAttribOp sFixedAttribConstant[] =
{
	NULL,
	RQ_NormalConst,
	RQ_ColorConst,
	RQ_TexCoordConst,
};

#else

// adapt normal pointer function so it matches the other pointer functions
static void APIENTRY glNormalPointerVV(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(size == 3);
	glNormalPointer(type, stride, pointer);
}

// fixed-function attribute array functions
// TO DO: use indices based on GL_*_ARRAY enum
typedef void (APIENTRY *FixedAttribArray)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static const FixedAttribArray sFixedAttribArray[] =
{
	glVertexPointer,
	glNormalPointerVV,
	glColorPointer,
	glTexCoordPointer,
};

// fixed-function attribute constant functions
// TO DO: use indices based on GL_*_ARRAY enum
typedef void (APIENTRY *FixedAttribConstant)(const GLfloat *v);
static const FixedAttribConstant sFixedAttribConstant[] =
{
	glVertex3fv,
	glNormal3fv,
	glColor4fv,
	glTexCoord2fv,
};

#endif

// set attrib count
void SetAttribCount(GLint aCount)
{
	if (sAttribCount == aCount)
		return;

	assert(sVertexCount == 0 && sIndexCount == 0);

	assert(sAttribCount < RENDER_MAX_ATTRIB);
	sAttribCount = aCount;
}

// set attribute format
void SetAttribFormat(GLint aIndex, GLuint aWidth, GLenum aType)
{
	if (aIndex < 0)
		return;

	if (aIndex < sAttribCount &&
		sAttribWidth[aIndex] == aWidth &&
		sAttribType[aIndex] == aType)
		return;

	assert(sVertexCount == 0 && sIndexCount == 0);

	if (sAttribCount < aIndex + 1)
		sAttribCount = aIndex + 1;
	sAttribWidth[aIndex] = aWidth;
	sAttribType[aIndex] = aType;
	switch (aType)
	{
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
		sAttribSize[aIndex] = aWidth;
		break;

	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
		sAttribSize[aIndex] = aWidth * 2;
		break;

	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
		sAttribSize[aIndex] = aWidth * 4;
		break;

	default:
		assert(false);
		break;
	}

	// init vertex format
	InitVertexFormat();
}

// set attribute constant
// (not bound to a buffer object)
void SetAttribConstantInternal(GLint aIndex, __m128 aValue)
{
	// switch to constant
	sAttribBuffer[aIndex] = 0;

	// set the constant attribute value
	sAttribValue[aIndex] = aValue;

	// TO DO: add "dirty" flag to attribute to avoid redundant changes?
#ifdef RENDER_USE_QUEUE
	// queue attrib value command
	if (sProgram)
		Expression::Append(sRenderQueue, RQ_AttribConst, aIndex, aValue);
	else
		Expression::Append(sRenderQueue, sFixedAttribConstant[aIndex], aValue);
#else
	if (sProgram)
	{
		glDisableVertexAttribArray(aIndex);
		glVertexAttrib4fv(aIndex, aValue.m128_f32);
	}
	else
	{
		glDisableClientState(sFixedAttrib[aIndex]);
		sFixedAttribConstant[aIndex](aValue.m128_f32);
	}
#endif
}
void SetAttribConstant(GLint aIndex, __m128 aValue)
{
	if (aIndex < 0)
		return;

	//if (aIndex < sAttribCount &&
	//	sAttribBuffer[aIndex] == 0 &&
	//	memcmp(sAttribValue[aIndex].m128_f32, aValue.m128_f32, sizeof(__m128)) == 0)
	//	return;

	// if not defined yet...
	if (sAttribCount < aIndex + 1)
	{
		// initialize to safe values
		sAttribCount = aIndex + 1;
		sAttribBuffer[aIndex] = 0;
	}

	// if the attribute is currently array...
	if (sAttribBuffer[aIndex] != 0)
	{
		// if building dynamic geometry...
		if (sVertexWorkFormat)
		{
			// flush dynamic geometry
			FlushDynamic();

			// init vertex format
			InitVertexFormat();
		}
	}

	// set the actual value
	SetAttribConstantInternal(aIndex, aValue);
}

// set attribute buffer
void SetAttribBufferInternal(GLint aIndex, BufferObject &aBuffer, GLuint aStride, GLuint aOffset)
{
#ifdef RENDER_USE_QUEUE
	// queue buffer command
	if (sVertexBuffer != &aBuffer)
	{
		sVertexBuffer = &aBuffer;
		Expression::Append(sRenderQueue, RQ_BindVertexBuffer, aBuffer.mHandle);
	}

	// queue attrib array command
	AttribArrayDesc desc;
	assert(aIndex <= UCHAR_MAX);
	desc.index = GLubyte(aIndex);
	desc.width = GLubyte(sAttribWidth[aIndex]);
	desc.type = GLubyte(sAttribType[aIndex] - GL_BYTE);
	assert(aStride <= UCHAR_MAX);
	desc.stride = GLubyte(aStride);
	desc.offset = aOffset;
	if (sProgram)
	{
		Expression::Append(sRenderQueue, RQ_AttribArray, desc);
	}
	else
	{
		Expression::Append(sRenderQueue, sFixedAttribArray[aIndex], desc);
	}
#else
	// bind buffer
	if (sVertexBuffer != &aBuffer)
	{
		sVertexBuffer = &aBuffer;
		glBindBuffer(GL_ARRAY_BUFFER, aBuffer.mHandle);
	}
	if (sProgram)
	{
		glEnableVertexAttribArray(aIndex);
		glVertexAttribPointer(aIndex, sAttribWidth[aIndex], sAttribType[aIndex], GL_TRUE, aStride, reinterpret_cast<const GLvoid *>(aOffset));
	}
	else
	{
		glEnableClientState(sFixedAttrib[aIndex]);
		sFixedAttribArray[aIndex](sAttribWidth[aIndex], sAttribType[aIndex], aStride, reinterpret_cast<const GLvoid *>(aOffset));
	}
#endif

	// switch to buffer
	sAttribBuffer[aIndex] = aBuffer.mHandle;
	sAttribStride[aIndex] = aStride;
	sAttribOffset[aIndex] = aOffset;
}
void SetAttribBuffer(GLint aIndex, BufferObject &aBuffer, GLuint aStride, GLuint aOffset)
{
	if (aIndex < 0)
		return;

	if (aIndex < sAttribCount &&
		sAttribBuffer[aIndex] == aBuffer.mHandle &&
		sAttribStride[aIndex] == aStride &&
		sAttribOffset[aIndex] == aOffset)
		return;

	// if not defined yet...
	if (sAttribCount < aIndex + 1)
	{
		// initialize to safe values
		sAttribCount = aIndex + 1;
		sAttribBuffer[aIndex] = 0;
		sAttribStride[aIndex] = 0;
		sAttribOffset[aIndex] = 0;
	}

	//assert(sVertexCount == 0 && sIndexCount == 0);
	assert(aBuffer.mHandle != 0);
	assert(aBuffer.mTarget == GL_ARRAY_BUFFER);
	//assert(aOffset < aBuffer.mEnd);

	// if the attribute is currently constant...
	if (sAttribBuffer[aIndex] == 0)
	{
		// if building dynamic geometry...
		if (sVertexWorkFormat)
		{
			// flush dynamic geometry
			FlushDynamic();

			// init vertex format
			InitVertexFormat();
		}
	}

	// set the actual values
	SetAttribBufferInternal(aIndex, aBuffer, aStride, aOffset);
}


// set index buffer
void SetIndexBuffer(BufferObject &aBuffer)
{
	assert(aBuffer.mTarget == GL_ELEMENT_ARRAY_BUFFER);
	if (sIndexBuffer != &aBuffer)
	{
		sIndexBuffer = &aBuffer;
#ifdef RENDER_USE_QUEUE
		Expression::Append(sRenderQueue, RQ_BindIndexBuffer, aBuffer.mHandle);
#else
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aBuffer.mHandle);
#endif
	}
}


//
// DRAW FUNCTIONS
//

// begin scene
void BeginScene(void)
{
	// clear render state
	ClearRenderState();
}

// end scene
void EndScene(void)
{
	// flush any pending geometry
	FlushDynamic();

#ifdef RENDER_USE_QUEUE
	// clear active buffer
	sVertexBuffer = NULL;
	sIndexBuffer = NULL;

	// reset shader program
	sProgram = 0;

	if (!sRenderQueue.empty())
	{
		// execute the render queue
		const unsigned int *begin = &sRenderQueue.front();
		const unsigned int *end = begin + sRenderQueue.size();
		Expression::Context context(begin);
		while (context.mStream < end)
			Expression::Evaluate<void>(context);

		// reset the render queue
		sRenderQueue.clear();
	}
#endif

	// reset dynamic buffers
	BufferSetData(sDynamicVertexBuffer, sDynamicVertexBuffer.mSize, NULL);
	sDynamicVertexBuffer.mStart = sDynamicVertexBuffer.mEnd = 0;
	BufferSetData(sDynamicIndexBuffer, sDynamicIndexBuffer.mSize, NULL);
	sDynamicIndexBuffer.mStart = sDynamicIndexBuffer.mEnd = 0;
}

// is dynamic mode active?
bool IsDynamicActive(void)
{
	return 
		sVertexCount > 0;
}

// pack attrib value
static void PackAttribValue(GLint aIndex, __m128 aValue)
{
	assert(aIndex >= 0 && aIndex < sAttribCount);

	// get displacement from packed vertex start
	register GLuint displace = sAttribDisplace[aIndex];
	register GLuint width = sAttribWidth[aIndex];
	assert(displace + sAttribSize[aIndex] < sizeof(sVertexPacked));

	// set the packed attribute value
	switch (sAttribType[aIndex])
	{
	case GL_BYTE:
		{
			register GLbyte *dst = reinterpret_cast<GLbyte *>(sVertexPacked + displace);
			//if (sAttribNormalize[aIndex])
				for (register GLuint i = 0; i < width; ++i)
					*dst++ = GLbyte(Clamp(xs_FloorToInt(aValue.m128_f32[i] * 0x80), CHAR_MIN, CHAR_MAX));
			//else
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLbyte(Clamp(xs_FloorToInt(aValue.m128_f32[i]), CHAR_MIN, CHAR_MAX));
		}
		break;

	case GL_UNSIGNED_BYTE:
		{
			register GLubyte *dst = reinterpret_cast<GLubyte *>(sVertexPacked + displace);
			//if (sAttribNormalize[aIndex])
				for (register GLuint i = 0; i < width; ++i)
					*dst++ = GLubyte(Clamp(xs_FloorToInt(aValue.m128_f32[i] * 0x100), 0, UCHAR_MAX));
			//else
			//	for (register GLuint i = 0; i < width; ++i)
			//		reinterpret_cast<GLubyte *>(sAttribPacked[aIndex])[i] = GLubyte(Clamp(xs_FloorToInt(aValue.m128_f32[i]), 0, UCHAR_MAX));
		}
		break;

	case GL_SHORT:
		{
			//register GLshort *dst = reinterpret_cast<GLshort *>(sVertexPacked + displace);
			//if (sAttribNormalize[aIndex])
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLshort(Clamp(xs_FloorToInt(aValue.m128_f32[i] * 0x8000), SHRT_MIN, SHRT_MAX));
			//else
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLshort(Clamp(xs_FloorToInt(aValue.m128_f32[i]), SHRT_MIN, SHRT_MAX));
		}
		break;

	case GL_UNSIGNED_SHORT:
		{
			//register GLushort *dst = reinterpret_cast<GLushort *>(sVertexPacked + displace);
			//if (sAttribNormalize[aIndex])
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLushort(Clamp(xs_FloorToInt(aValue.m128_f32[i] * 0x10000), 0, USHRT_MAX));
			//else
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLushort(Clamp(xs_FloorToInt(aValue.m128_f32[i]), 0, USHRT_MAX));
		}
		break;

	case GL_INT:
		{
			//register GLint *dst = reinterpret_cast<GLint *>(sVertexPacked + displace);
			//if (sAttribNormalize[aIndex])
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLint(Clamp(xs_FloorToInt(aValue.m128_f32[i] * 0x80000000), INT_MIN, INT_MAX));
			//else
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLint(Clamp(xs_FloorToInt(aValue.m128_f32[i]), INT_MIN, INT_MAX));
		}
		break;

	case GL_UNSIGNED_INT:
		{
			//register GLuint *dst = reinterpret_cast<GLuint *>(sVertexPacked + displace);
			//if (sAttribNormalize[aIndex])
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLuint(Clamp<GLuint>(xs_FloorToInt(aValue.m128_f32[i] * 0x100000000), 0, UINT_MAX));
			//else
			//	for (register GLuint i = 0; i < width; ++i)
			//		*dst++ = GLuint(Clamp<GLuint>(xs_FloorToInt(aValue.m128_f32[i]), 0, UINT_MAX));
		}
		break;

	case GL_FLOAT:
		{
			// direct copy
			memcpy(sVertexPacked + displace, &aValue, width * sizeof(GLfloat));
		}
		break;
	}
}

// set up attributes for use with dynamic buffer
static void SetupDynamicAttribs(GLuint aFormat, GLuint aOffset)
{
	// get vertex stride
	GLuint vertexsize = 0;
	for (int index = 0; index < sAttribCount; ++index)
	{
		if (aFormat & (1 << index))
		{
			vertexsize += sAttribSize[index];
		}
	}

	// set up attribute pointers and values
	GLuint offset = aOffset;
	for (int index = 0; index < sAttribCount; ++index)
	{
		if (aFormat & (1 << index))
		{
			SetAttribBufferInternal(index, sDynamicVertexBuffer, vertexsize, offset);
			offset += sAttribSize[index];
		}
		else
		{
			SetAttribConstantInternal(index, sAttribValue[index]);
		}
	}
}

// set work buffer vertex format
void SetWorkFormat(GLuint aFormat)
{
	if (sVertexWorkFormat == aFormat)
		return;

	// flush previous geometry
	FlushDynamic();

	// save vertex format
	sVertexWorkFormat = aFormat;

#ifdef DEBUG
	// pave the packed vertex buffer
	memset(sVertexPacked, -1, sizeof(sVertexPacked));
#endif

	// compute work vertex size and interleaved offsets
	sVertexWorkSize = 0;
	for (int index = 0; index < sAttribCount; ++index)
	{
		if (aFormat & (1 << index))
		{
			// add to interleaved offset
			sAttribDisplace[index] = sVertexWorkSize;
			sVertexWorkSize += sAttribSize[index];
			PackAttribValue(index, sAttribValue[index]);
		}
	}
}

// get work format
GLuint GetWorkFormat(void)
{
	return sVertexWorkFormat;
}

// set draw mode
void SetDrawMode(GLenum aDrawMode)
{
	if (sDrawMode == aDrawMode)
		return;

	// flush previous geometry
	FlushDynamic();

	// set the draw mode
	sDrawMode = aDrawMode;

	// save base vertex
	sVertexBase = sVertexCount;
}

/*
// set attrib value
void SetAttribValue(GLint aIndex, float const *aValue, GLuint count)
{
	if (aIndex < 0)
		return;

	// set the constant attribute value
	memcpy(&sAttribValue[aIndex], aValue, count * sizeof(aValue));
}
*/

// set attrib value
void SetAttribValue(GLint aIndex, __m128 aValue)
{
	if (aIndex < 0)
		return;

	// set the constant attribute value
	sAttribValue[aIndex] = aValue;

	// update the packed vertex
	PackAttribValue(aIndex, aValue);
}

// add vertex to work buffer
void AddVertex(void)
{
	assert(sVertexWorkSize > 0);
	assert(sVertexUsed + sVertexWorkSize / sizeof(float) < sVertexLimit);

	// copy the packed vertex state
	__movsd(
		reinterpret_cast<unsigned long *>(sVertexWork + sVertexUsed),
		reinterpret_cast<unsigned long *>(sVertexPacked),
		sVertexWorkSize / sizeof(unsigned long)
		);
	sVertexUsed += sVertexWorkSize / sizeof(float);

	// get ready for the next vertex
	++sVertexCount;
}

extern void *AllocVertices(GLuint aCount)
{
	if (sVertexUsed + aCount * sVertexWorkSize / sizeof(float) >= sVertexLimit)
	{
		FlushDynamic();
	}
	void *dest = sVertexWork + sVertexUsed;
	sVertexUsed += aCount * sVertexWorkSize / sizeof(float);
	sVertexCount += aCount;
	return dest;
}

extern void *AllocIndices(GLuint aCount)
{
	if (sIndexCount + aCount >= sIndexLimit)
	{
		FlushDynamic();
	}
	void *dest = sIndexWork + sIndexCount;
	sIndexCount += aCount;
	return dest;
}



void AddVertices(GLuint aCount, const void *aData)
{
	void *dest = AllocVertices(aCount);
	memcpy(dest, aData, aCount * sVertexWorkSize);
}

void IndexLines(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 0; i < aCount; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
	}
}

void IndexLineLoop(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 0; i < aCount - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 1);
	}
	sIndexWork[sIndexCount++] = unsigned short(aStart + aCount - 1);
	sIndexWork[sIndexCount++] = unsigned short(aStart);
}

void IndexLineStrip(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 0; i < aCount - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 1);
	}
}

void IndexTriangles(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 0; i < aCount; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
	}
}

void IndexTriangleStrip(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 0; i < aCount - 2; ++i)
	{
		const int odd = i & 1;
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + odd);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 1 - odd);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 2);
	}
}

void IndexTriangleFan(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 1; i < aCount - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 1);
	}
}

void IndexQuads(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 0; i < aCount; i += 4)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 1);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 2);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 2);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 3);
	}
}

void IndexQuadStrip(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 0; i < aCount - 2; i += 2)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 1);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 3);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 3);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 2);
	}
}

void IndexPolygon(GLuint aStart, GLuint aCount)
{
	for (register GLuint i = 1; i < aCount - 1; ++i)
	{
		sIndexWork[sIndexCount++] = unsigned short(aStart);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i);
		sIndexWork[sIndexCount++] = unsigned short(aStart + i + 1);
	}
}

// get the current vertex count
GLuint GetVertexCount(void)
{
	return sVertexCount;
}

// get the current index count
GLuint GetIndexCount(void)
{
	return sIndexCount;
}

// draw non-indexed primitive
void DrawArrays(GLenum aDrawMode, /*GLuint aVertexFirst,*/ GLuint aVertexCount)
{
#ifdef RENDER_USE_QUEUE
	DrawArraysDesc desc;
	desc.mode = GLubyte(sDrawMode);
	desc.count = GLushort(sVertexCount);
	Expression::Append(sRenderQueue, RQ_DrawArrays, desc);
#else
	glDrawArrays(sDrawMode, 0, sVertexCount);
#endif
}

// draw indexed primitive
void DrawElements(GLenum aDrawMode, /*GLuint aVertexFirst,*/ GLuint aVertexCount, GLuint aIndexCount, GLuint aIndexOffset)
{
#ifdef RENDER_USE_QUEUE
	DrawElementsDesc desc;
	assert(sIndexBuffer->mHandle <= UCHAR_MAX);
	desc.mode = GLubyte(aDrawMode);
	assert(sIndexCount <= USHRT_MAX);
	desc.count = GLushort(aIndexCount);
	desc.offset = aIndexOffset;
	Expression::Append(sRenderQueue, RQ_DrawElements, desc);
#else
	glDrawElements(aDrawMode, aIndexCount, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid *>(aIndexOffset));
#endif
}

// flush dynamic geometry
void FlushDynamic(void)
{
	// do nothing if the work buffer is empty
	if (sVertexCount == 0)
		return;

#if 0
	// sync opengl matrix
	// how to handle this correctly?
	//SetUniformMatrix4(sUniformModelView, IdentityGet());
	assert(sProgram == 0);
#ifdef DYNAMIC_GEOMETRY_IN_VIEW_SPACE
	SetUniformMatrix4(sUniformModelView, IdentityGet());
#else
	SetUniformMatrix4Internal(GL_MODELVIEW, ViewGet());
#endif
#endif

	// make sure work buffer is still valid
	assert(sVertexWorkFormat != 0);
	assert(sVertexWorkSize != 0);

	// set up dynamic attributes
	SetupDynamicAttribs(sVertexWorkFormat, sDynamicVertexBuffer.mStart);

	// copy vertex work buffer to the array buffer
	BufferAppendData(sDynamicVertexBuffer, sVertexUsed * sizeof(float), sVertexWork);

	// emit a draw call
	if (sIndexCount > 0)
	{
		// copy index work buffer to the array buffer
		BufferAppendData(sDynamicIndexBuffer, sIndexCount * sizeof(unsigned short), sIndexWork);

		// set index buffer
		SetIndexBuffer(sDynamicIndexBuffer);

		// draw indexed primitive
		DrawElements(sDrawMode, sVertexCount, sIndexCount, sDynamicIndexBuffer.mStart);
	}
	else
	{
		// draw non-indexed primitive
		DrawArrays(sDrawMode, sVertexCount);
	}

	// reset work buffer
	InitWorkBuffer();

	// get ready for the next batch
	sDynamicVertexBuffer.mStart = sDynamicVertexBuffer.mEnd;
	sDynamicIndexBuffer.mStart = sDynamicIndexBuffer.mEnd;
}

void BindTexture(GLuint aTexture)
{
	if (sTexture == aTexture)
		return;

	FlushDynamic();

	sTexture = aTexture;

#ifdef RENDER_USE_QUEUE
	Expression::Append(sRenderQueue, RQ_BindTexture, aTexture);
#else
	if (aTexture)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, aTexture);
#endif
}

void CopyTexSubImage(GLint aLevel, GLint aXOffset, GLint aYOffset, GLint aX, GLint aY, GLsizei aWidth, GLsizei aHeight)
{
#ifdef RENDER_USE_QUEUE
	Expression::Append(sRenderQueue, RQ_CopyTexSubImage, aLevel, aXOffset, aYOffset);
	Expression::Append(sRenderQueue, aX, aY, aWidth, aHeight);
#else
	glCopyTexSubImage2D(GL_TEXTURE_2D, aLevel, aXOffset, aYOffset, aX, aY, aWidth, aHeight);
#endif
}

void ClearFrame(void)
{
#ifdef RENDER_USE_QUEUE
	Expression::Append(sRenderQueue, RQ_ClearFrame);
#else
	// clear the screen
	glClear(
		GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_TEST
		| GL_DEPTH_BUFFER_BIT
#endif
		);
#endif
}

// set clear color
void SetClearColor(const Color4 &aColor)
{
	// should this be queued?
	sClearColor = aColor;

	glClearColor( aColor.r, aColor.g, aColor.b, 0 );
}

// get clear color
const Color4 &GetClearColor(void)
{
	return sClearColor;
}

// set fog enable
void SetFogEnabled(bool bEnable)
{
	sFogEnabled = bEnable;

#ifdef SUPPORT_FIXED_FUNCTION
	// fixed-function
	if (bEnable)
		glEnable(GL_FOG);
	else
		glDisable(GL_FOG);
#endif
}


// get fog enable
bool GetFogEnabled(void)
{
	return sFogEnabled;
}

// set fog color
void SetFogColor(const Color4 &aColor)
{
	sFogColor = aColor;

#ifdef SUPPORT_FIXED_FUNCTION
	// fixed-function
	glFogfv( GL_FOG_COLOR, aColor );
#endif
}

// get fog color
const Color4 &GetFogColor(void)
{
	return sFogColor;
}

void SetFogStart(float aStart)
{
	sFogStart = aStart;

#ifdef SUPPORT_FIXED_FUNCTION
	// fixed-function
	glFogf( GL_FOG_START, aStart );
#endif
}

void SetFogEnd(float aEnd)
{
	sFogEnd = aEnd;

#ifdef SUPPORT_FIXED_FUNCTION
	// fixed-function
	glFogf( GL_FOG_END, aEnd );
#endif
}

float GetFogStart(void)
{
	return sFogStart;
}

float GetFogEnd(void)
{
	return sFogEnd;
}
