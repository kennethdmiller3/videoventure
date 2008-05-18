#include "StdAfx.h"
#include "World.h"
#include "Entity.h"

namespace Database
{
	namespace Loader
	{
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
				DebugPrint("Import %s\n", name);
				TiXmlDocument document(name);
				document.LoadFile();

				// process child elements
				if (const TiXmlElement *root = document.FirstChildElement())
					ProcessWorldItems(root);
			}
		}
		importloader;
	}
}

void ProcessWorldItem(const TiXmlElement *element)
{
	const char *value = element->Value();
	const char *name = element->Attribute("name");
	DebugPrint("Processing %s (%s)\n", element->Value(), name);

	// process world item
	const Database::Loader::Entry &configure = Database::Loader::GetConfigure(Hash(value));
	if (configure)
		configure(Hash(name), element);
	else
		DebugPrint("Unrecognized tag \"%s\"\n", value);
}

void ProcessWorldItems(const TiXmlElement *element)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessWorldItem(child);
	}
}
