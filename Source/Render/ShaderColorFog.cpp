#include "StdAfx.h"
#include "Render.h"
#include "ShaderColorFog.h"

namespace ShaderColorFog
{
	// vertex shader source
	static const GLchar * const sVertexSource =
		"#version 130\n"
		"\n"
		"uniform mat4 modelviewproj;\n"
		"uniform mat4 modelview;\n"
		"\n"
		"in vec3 position;\n"
		"in vec4 color;\n"
		"\n"
		"out vec3 vsposition;\n"
		"out vec4 vscolor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = modelviewproj * vec4(position, 1.0);\n"
		"    vsposition = mat4x3(modelview) * vec4(position, 1.0);\n"
		"    vscolor = color;\n"
		"}\n";

	// fragment shader source
	static const GLchar * const sFragmentSource =
		"#version 130\n"
		"\n"
		"uniform vec4 fogcolor;\n"
		"uniform vec4 fogparams;\n"
		"\n"
		"in vec3 vsposition;\n"
		"in vec4 vscolor;\n"
		"\n"
		"out vec4 fragmentcolor;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"    fragmentcolor = vscolor;\n"
		"    float fogvalue = fogparams.x * smoothstep(fogparams.y, fogparams.z, length(vsposition));\n"
		"    fragmentcolor.xyz = mix(fragmentcolor.xyz, fogcolor.xyz, fogvalue);\n"
		"}\n";

	// shader program
	GLuint gProgramId;
	static GLuint sVertexId;
	static GLuint sFragmentId;

	// uniform locations
	GLint gUniformModelViewProj;
	GLint gUniformModelView;
	GLint gUniformFogColor;
	GLint gUniformFogParams;

	// attribute locations
	GLint gAttribPosition;
	GLint gAttribColor;

	void Init(void)
	{
		// create program from source
		sVertexId = CreateVertexShader(sVertexSource);
		sFragmentId = CreateFragmentShader(sFragmentSource);
		gProgramId = CreateProgram(sVertexId, sFragmentId);

		// get uniform location
		gUniformModelViewProj = glGetUniformLocation(gProgramId, "modelviewproj");
		gUniformModelView = glGetUniformLocation(gProgramId, "modelview");
		gUniformFogColor = glGetUniformLocation(gProgramId, "fogcolor");
		gUniformFogParams = glGetUniformLocation(gProgramId, "fogparams");

		// get attribute locations
		gAttribPosition = glGetAttribLocation(gProgramId, "position");
		gAttribColor = glGetAttribLocation(gProgramId, "color");
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
