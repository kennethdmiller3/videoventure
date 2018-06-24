#include "StdAfx.h"
#include "World.h"
#include "Entity.h"
#include "Collidable.h"
#include "Render.h"

namespace Database
{
	namespace Loader
	{
		static void WorldConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// set up the collidable world
			float aMinX = -2048, aMinY = -2048, aMaxX = 2048, aMaxY = 2048;
			bool aWall = true;
			element->QueryFloatAttribute("xmin", &aMinX);
			element->QueryFloatAttribute("ymin", &aMinY);
			element->QueryFloatAttribute("xmax", &aMaxX);
			element->QueryFloatAttribute("ymax", &aMaxY);
			element->QueryBoolAttribute("wall", &aWall);
			Collidable::WorldInit(aMinX, aMinY, aMaxX, aMaxY, aWall != 0);

			// recurse on children
			ConfigureWorldItems(element);
		}
		Configure worldconfigure(0x37a3e893 /* "world" */, WorldConfigure);

		static void ImportConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// level configuration
			const char *name = element->Attribute("name");
			tinyxml2::XMLDocument document;
			if (document.LoadFile(name) != tinyxml2::XML_SUCCESS)
				DebugPrint("error loading import file \"%s\": %s\n", name, document.ErrorStr());

			// process child elements
			if (const tinyxml2::XMLElement *root = document.FirstChildElement())
				ConfigureWorldItem(root);
		}
		Configure importconfigure(0x112a90d4 /* "import" */, ImportConfigure);

		static void FogConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// set up depth fog
			bool enable = false;
			if (element->QueryBoolAttribute("enable", &enable) == tinyxml2::XML_SUCCESS)
				SetFogEnabled(enable);

			switch (Hash(element->Attribute("mode")))
			{
			case 0:
				break;

			default:
			case 0xd00594c0 /* "linear" */:
				{
#ifdef SUPPORT_FIXED_FUNCTION
					glFogi( GL_FOG_MODE, GL_LINEAR );
#endif

					float start = 0;
					if (element->QueryFloatAttribute("start", &start) == tinyxml2::XML_SUCCESS)
						SetFogStart(start);

					float end = 1;
					if (element->QueryFloatAttribute("end", &end) == tinyxml2::XML_SUCCESS)
						SetFogEnd(end);
				}
				break;

#if 0
			case 0x72a68728 /* "exp" */:
				{
					glFogi( GL_FOG_MODE, GL_EXP );

					float density = 1.0f;
					if (element->QueryFloatAttribute("density", &density) == tinyxml2::XML_SUCCESS)
						glFogf( GL_FOG_DENSITY, density );
				}
				break;

			case 0x9626adee /* "exp2" */:
				{
					glFogi( GL_FOG_MODE, GL_EXP2 );

					float density = 1.0f;
					if (element->QueryFloatAttribute("density", &density) == tinyxml2::XML_SUCCESS)
						glFogf( GL_FOG_DENSITY, density );
				}
				break;
#endif
			}

			Color4 fogColor(0.0f, 0.0f, 0.0f, 1.0f);
			element->QueryFloatAttribute("r", &fogColor.r);
			element->QueryFloatAttribute("g", &fogColor.g);
			element->QueryFloatAttribute("b", &fogColor.b);
			element->QueryFloatAttribute("a", &fogColor.a);
			SetFogColor(fogColor);
			SetClearColor(fogColor);
		}
		Configure fogconfigure(0xa1f3723f /* "fog" */, FogConfigure);
	}
}

void ConfigureWorldItem(const tinyxml2::XMLElement *element)
{
	const char *value = element->Value();
	const char *name = element->Attribute("name");

	// get item identifier
	Database::Key id = Hash(name);

	// process world item
	const Database::Loader::Entry &configure = Database::Loader::Configure::Get(Hash(value));
	if (configure)
	{
		DebugPrint("processing %s \"%s\"\n", value, name);
		configure(id, element);
	}
	else
	{
		DebugPrint("skipping %s \"%s\"\n", value, name);
	}

	// assign the name
	if (name)
		Database::name.Put(id, name);
}

void ConfigureWorldItems(const tinyxml2::XMLElement *element)
{
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureWorldItem(child);
	}
}
