#include "StdAfx.h"
#include "Render.h"
#include "ShaderModulate.h"

namespace ShaderModulate
{
	// vertex shader source
	static const GLchar * const sVertexSource =
		"#version 130\n"
		"\n"
		"uniform mat4 modelviewproj;\n"
		"\n"
		"in vec3 position;\n"
		"in vec4 color;\n"
		"in vec2 texcoord;\n"
		"\n"
		"out vec4 vscolor;\n"
		"out vec2 vstexcoord;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = modelviewproj * vec4(position, 1.0);\n"
		"    vscolor = color;\n"
		"    vstexcoord = texcoord;\n"
		"}\n";

	// fragment shader source
	static const GLchar * const sFragmentSource =
		"#version 130\n"
		"\n"
		"uniform sampler2D sampler;\n"
		"\n"
		"in vec4 vscolor;\n"
		"in vec2 vstexcoord;\n"
		"\n"
		"out vec4 fragmentcolor;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"    fragmentcolor = vscolor * texture(sampler, vstexcoord);\n"
		"}\n";

	// shader program
	GLuint gProgramId;
	static GLuint sVertexId;
	static GLuint sFragmentId;

	// uniform locations
	GLint gUniformModelViewProj;

	// attribute locations
	GLint gAttribPosition;
	GLint gAttribColor;
	GLint gAttribTexCoord;

	void Init(void)
	{
		// create program from source
		sVertexId = CreateVertexShader(sVertexSource);
		sFragmentId = CreateFragmentShader(sFragmentSource);
		gProgramId = CreateProgram(sVertexId, sFragmentId);

		// get uniform location
		gUniformModelViewProj = glGetUniformLocation(gProgramId, "modelviewproj");

		// get attribute locations
		gAttribPosition = glGetAttribLocation(gProgramId, "position");
		gAttribColor = glGetAttribLocation(gProgramId, "color");
		gAttribTexCoord = glGetAttribLocation(gProgramId, "texcoord");
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
