#include "StdAfx.h"
#include "Render.h"
#include "ShaderColor.h"

namespace ShaderColor
{
	// color vertex shader
	// vertex position: transform by modelview and projection
	// vertex color: pass through unmodified
	static const GLchar * const sVertexSource =
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

	// color fragment shader
	static const GLchar * const sFragmentSource =
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
	GLuint gProgramId;
	static GLuint sVertexId;
	static GLuint sFragmentId;

	// uniform locations
	GLint gUniformModelViewProj;

	void Init(void)
	{
		// create program from source
		sVertexId = CreateVertexShader(sVertexSource);
		sFragmentId = CreateFragmentShader(sFragmentSource);
		gProgramId = CreateProgram(sVertexId, sFragmentId);
		glBindAttribLocation(gProgramId, ATTRIB_INDEX_POSITION, "position");
		glBindAttribLocation(gProgramId, ATTRIB_INDEX_COLOR, "color");
		LinkProgram(gProgramId);

		// get uniform location
		gUniformModelViewProj = glGetUniformLocation(gProgramId, "modelviewproj");
	}

	void Cleanup(void)
	{
		glDeleteProgram(gProgramId);
		gProgramId = 0;
		glDeleteShader(sFragmentId);
		sFragmentId = 0;
		glDeleteShader(sVertexId);
		sVertexId = 0;
	}

	void PreReset(void)
	{
		Cleanup();
	}

	void PostReset(void)
	{
		Init();
	}
}
