#include "StdAfx.h"
#include "Render.h"
#include "ShaderModulateFog.h"

namespace ShaderModulateFog
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
		"in vec2 texcoord;\n"
		"\n"
		"out vec3 vsposition;\n"
		"out vec4 vscolor;\n"
		"out vec2 vstexcoord;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = modelviewproj * vec4(position, 1.0);\n"
		"    vsposition = mat4x3(modelview) * vec4(position, 1.0);\n"
		"    vscolor = color;\n"
		"    vstexcoord = texcoord;\n"
		"}\n";

	// fragment shader source
	static const GLchar * const sFragmentSource =
		"#version 130\n"
		"\n"
		"uniform sampler2D sampler;\n"
		"uniform vec4 fogcolor;\n"
		"uniform vec4 fogparams;\n"
		"\n"
		"in vec3 vsposition;\n"
		"in vec4 vscolor;\n"
		"in vec2 vstexcoord;\n"
		"\n"
		"out vec4 fragmentcolor;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"    fragmentcolor = vscolor * texture(sampler, vstexcoord);\n"
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

	void Init(void)
	{
		// create program from source
		sVertexId = CreateVertexShader(sVertexSource);
		sFragmentId = CreateFragmentShader(sFragmentSource);
		gProgramId = CreateProgram(sVertexId, sFragmentId);
		glBindAttribLocation(gProgramId, ATTRIB_INDEX_POSITION, "position");
		glBindAttribLocation(gProgramId, ATTRIB_INDEX_COLOR, "color");
		glBindAttribLocation(gProgramId, ATTRIB_INDEX_TEXCOORD, "texcoord");
		LinkProgram(gProgramId);

		// get uniform location
		gUniformModelViewProj = glGetUniformLocation(gProgramId, "modelviewproj");
		gUniformModelView = glGetUniformLocation(gProgramId, "modelview");
		gUniformFogColor = glGetUniformLocation(gProgramId, "fogcolor");
		gUniformFogParams = glGetUniformLocation(gProgramId, "fogparams");
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
