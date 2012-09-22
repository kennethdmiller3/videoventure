#pragma once

// shader program descriptor
struct ProgramTemplate
{
	// vertex shader id
	// Database::vertexshadertemplate contains shader source code
	// Database::vertexshader contains shader handle
	unsigned int mVertex;

	// fragment shader id
	// Database::fragmentshadertemplate contains shader source code
	// Database::fragmentshader contains shader handle
	unsigned int mFragment;

	// map attribute names to locations
	Database::Typed<unsigned int> mAttrib;
};

extern void CleanupShaders(void);
extern void RebuildShaders(void);

namespace Database
{
	extern Typed<std::string> vertexshadertemplate;
	extern Typed<std::string> fragmentshadertemplate;
	extern Typed<ProgramTemplate> programtemplate;

	extern Typed<GLuint> vertexshader;
	extern Typed<GLuint> fragmentshader;
	extern Typed<GLuint> program;
}
