#include "StdAfx.h"

#include "Shader.h"
#include "Render.h"

namespace Database
{
	Typed<std::string> vertexshadertemplate(0x41c4e094 /* "vertexshadertemplate" */);
	Typed<std::string> fragmentshadertemplate(0x0f59759a /* "fragmentshadertemplate" */);
	Typed<ProgramTemplate> programtemplate(0x5a0588d9 /* "programtemplate" */);

	Typed<GLuint> vertexshader(0x4f4ccf52 /* "vertexshader" */);
	Typed<GLuint> fragmentshader(0x65237bf0 /* "fragmentshader" */);
	Typed<GLuint> program(0x3d8466cb /* "program" */);

	namespace Loader
	{
		// read shader source from a file
		static bool ReadShaderFromFile(std::string &shader, const char *name)
		{
			FILE *file = fopen(name, "r");
			if (file == NULL)
			{
				DebugPrint("error loading shader file \"%s\": %s\n", name);
				return false;
			}

			// get file length
			fseek(file, 0, SEEK_END);
			size_t length = ftell(file);
			fseek(file, 0, SEEK_SET);

			// read contents
			char *buffer = static_cast<char *>(_alloca(length));
			fread(buffer, 1, length, file);
			shader.assign(buffer, length);

			fclose(file);

			return true;
		}

		static void VertexShaderConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// get vertex shader source...
			std::string &vertexsource = Database::vertexshadertemplate.Open(aId);
			if (const char *name = element->Attribute("file"))
			{
				// ...from the named file
				ReadShaderFromFile(vertexsource, name);
			}
			else if (const char *text = element->GetText())
			{
				// ...from the element text
				vertexsource.assign(text);
			}

			// create the vertex shader
			const GLuint vertexhandle = CreateVertexShader(vertexsource.c_str());
			Database::vertexshader.Put(aId, vertexhandle);

			// done with shader template
			Database::vertexshadertemplate.Close(aId);
		}
		Configure vertexshaderconfigure(0x4f4ccf52 /* "vertexshader" */, VertexShaderConfigure);

		static void FragmentShaderConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// get fragment shader source...
			std::string &fragmentsource = Database::fragmentshadertemplate.Open(aId);
			if (const char *name = element->Attribute("file"))
			{
				// ...from the named file
				ReadShaderFromFile(fragmentsource, name);
			}
			else if (const char *text = element->GetText())
			{
				// ...from the element text
				fragmentsource.assign(text);
			}

			// create the fragment shader
			const GLuint fragmenthandle = CreateFragmentShader(fragmentsource.c_str());
			Database::fragmentshader.Put(aId, fragmenthandle);

			// done with shader template
			Database::fragmentshadertemplate.Close(aId);
		}
		Configure fragmentshaderconfigure(0x65237bf0 /* "fragmentshader" */, FragmentShaderConfigure);

		static void ProgramConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// get program template
			ProgramTemplate &program = Database::programtemplate.Open(aId);

			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
			{
				switch (Hash(child->Value()))
				{
				case 0x4f4ccf52 /* "vertexshader" */:
					if (const char *name = element->Attribute("name"))
					{
						// get the named vertex shader
						program.mVertex = Hash(name);
					}
					else
					{
						// configure local vertex shader
						VertexShaderConfigure(aId, child);
						program.mVertex = aId;
					}
					break;

				case 0x65237bf0 /* "fragmentshader" */:
					if (const char *name = element->Attribute("name"))
					{
						// get the named fragment shader
						program.mFragment = Hash(name);
					}
					else
					{
						// configure local fragment shader
						FragmentShaderConfigure(aId, child);
						program.mFragment = aId;
					}
					break;
				}
			}

			// create shader program
			GLuint vertexhandle = Database::vertexshader.Get(program.mVertex);
			GLuint fragmenthandle = Database::fragmentshader.Get(program.mFragment);
			GLuint programhandle = CreateProgram(vertexhandle, fragmenthandle);
			Database::program.Put(aId, programhandle);

			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
			{
				switch (Hash(child->Value()))
				{
				case 0xd5cd8099 /* "attrib" */:
					if (const char *name = element->Attribute("name"))
					{
						// find the location of the named attribute
						// (-1 if it wasn't found or was optimized out)
						int location = glGetAttribLocation(programhandle, name);
						program.mAttrib.Put(Hash(name), location);
					}
					break;
				}
			}

			// done with program template
			Database::programtemplate.Close(aId);
		}
		Configure programconfigure(0x3d8466cb /* "program" */, ProgramConfigure);
	}
}

void CleanupShaders(void)
{
	// for each entry in the program database
	for (Database::Typed<GLuint>::Iterator itor(&Database::program); itor.IsValid(); ++itor)
	{
		// delete the program
		DeleteProgram(itor.GetValue());
	}

	// for each entry in the vertex shader database
	for (Database::Typed<GLuint>::Iterator itor(&Database::vertexshader); itor.IsValid(); ++itor)
	{
		// delete the shader
		DeleteShader(itor.GetValue());
	}

	// for each entry in the fragment shader database
	for (Database::Typed<GLuint>::Iterator itor(&Database::fragmentshader); itor.IsValid(); ++itor)
	{
		// delete the shader
		DeleteShader(itor.GetValue());
	}
}

void RebuildShaders(void)
{
	// for each entry in the vertex shader template database
	for (Database::Typed<std::string>::Iterator itor(&Database::vertexshadertemplate); itor.IsValid(); ++itor)
	{
		// recreate the vertex shader
		const Database::Key id = itor.GetKey();
		const std::string &vertexsource = itor.GetValue();
		const GLuint vertexhandle = CreateVertexShader(vertexsource.c_str());
		Database::vertexshader.Put(id, vertexhandle);
	}

	// for each entry in the fragment shader template database
	for (Database::Typed<std::string>::Iterator itor(&Database::fragmentshadertemplate); itor.IsValid(); ++itor)
	{
		// recreate the fragment shader
		const Database::Key id = itor.GetKey();
		const std::string &fragmentsource = itor.GetValue();
		const GLuint fragmenthandle = CreateFragmentShader(fragmentsource.c_str());
		Database::fragmentshader.Put(id, fragmenthandle);
	}

	// for each entry in the shader program template database
	for (Database::Typed<ProgramTemplate>::Iterator itor(&Database::programtemplate); itor.IsValid(); ++itor)
	{
		// recreate shader program
		const Database::Key id = itor.GetKey();
		const ProgramTemplate &program = itor.GetValue();
		GLuint vertexhandle = Database::vertexshader.Get(program.mVertex);
		GLuint fragmenthandle = Database::fragmentshader.Get(program.mFragment);
		GLuint programhandle = CreateProgram(vertexhandle, fragmenthandle);
		Database::program.Put(id, programhandle);
	}
}
