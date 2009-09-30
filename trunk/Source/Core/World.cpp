#include "StdAfx.h"
#include "World.h"
#include "Entity.h"
#include "Collidable.h"

namespace Database
{
	namespace Loader
	{
		class WorldLoader
		{
		public:
			WorldLoader()
			{
				AddConfigure(0x37a3e893 /* "world" */, Entry(this, &WorldLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// set up the collidable world
				float aMinX = -2048, aMinY = -2048, aMaxX = 2048, aMaxY = 2048;
				int aWall = 1;
				element->QueryFloatAttribute("xmin", &aMinX);
				element->QueryFloatAttribute("ymin", &aMinY);
				element->QueryFloatAttribute("xmax", &aMaxX);
				element->QueryFloatAttribute("ymax", &aMaxY);
				element->QueryIntAttribute("wall", &aWall);
				Collidable::WorldInit(aMinX, aMinY, aMaxX, aMaxY, aWall != 0);

				// recurse on children
				ConfigureWorldItems(element);
			}
		}
		worldloader;

		class ImportLoader
		{
		public:
			ImportLoader()
			{
				AddConfigure(0x112a90d4 /* "import" */, Entry(this, &ImportLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// level configuration
				const char *name = element->Attribute("name");
				TiXmlDocument document(name);
				if (!document.LoadFile())
					DebugPrint("error loading import file \"%s\": %s\n", name, document.ErrorDesc());

				// process child elements
				if (const TiXmlElement *root = document.FirstChildElement())
					ConfigureWorldItem(root);
			}
		}
		importloader;

		class FogLoader
		{
		public:
			FogLoader()
			{
				AddConfigure(0xa1f3723f /* "fog" */, Entry(this, &FogLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// set up depth fog
				int enable = 0;
				if (element->QueryIntAttribute("enable", &enable) == TIXML_SUCCESS)
				{
					if (enable)
						glEnable( GL_FOG );
					else
						glDisable( GL_FOG );
				}
				glHint( GL_FOG_HINT, GL_DONT_CARE );

				switch (Hash(element->Attribute("mode")))
				{
				case 0:
					break;

				default:
				case 0xd00594c0 /* "linear" */:
					{
						glFogi( GL_FOG_MODE, GL_LINEAR );

						float start = 0;
						if (element->QueryFloatAttribute("start", &start) == TIXML_SUCCESS)
							glFogf( GL_FOG_START, start );

						float end = 1;
						if (element->QueryFloatAttribute("end", &end) == TIXML_SUCCESS)
							glFogf( GL_FOG_END, end );
					}
					break;

				case 0x72a68728 /* "exp" */:
					{
						glFogi( GL_FOG_MODE, GL_EXP );

						float density = 1.0f;
						if (element->QueryFloatAttribute("density", &density) == TIXML_SUCCESS)
							glFogf( GL_FOG_DENSITY, density );
					}
					break;

				case 0x9626adee /* "exp2" */:
					{
						glFogi( GL_FOG_MODE, GL_EXP2 );

						float density = 1.0f;
						if (element->QueryFloatAttribute("density", &density) == TIXML_SUCCESS)
							glFogf( GL_FOG_DENSITY, density );
					}
					break;
				}

				GLfloat fogColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				element->QueryFloatAttribute("r", &fogColor[0]);
				element->QueryFloatAttribute("g", &fogColor[1]);
				element->QueryFloatAttribute("b", &fogColor[2]);
				element->QueryFloatAttribute("a", &fogColor[3]);
				glFogfv( GL_FOG_COLOR, fogColor );
				glClearColor( fogColor[0], fogColor[1], fogColor[2], 0 );
			}
		}
		fogloader;
	}
}

void ConfigureWorldItem(const TiXmlElement *element)
{
	const char *value = element->Value();
	const char *name = element->Attribute("name");
	if (name)
		Database::name.Put(Hash(name), name);
	DebugPrint("Processing %s (%s)\n", element->Value(), name);

	// process world item
	const Database::Loader::Entry &configure = Database::Loader::GetConfigure(Hash(value));
	if (configure)
		configure(Hash(name), element);
	else
		DebugPrint("Unrecognized tag \"%s\"\n", value);
}

void ConfigureWorldItems(const TiXmlElement *element)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureWorldItem(child);
	}
}
