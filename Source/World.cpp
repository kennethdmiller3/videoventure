#include "StdAfx.h"
#include "World.h"
#include "Entity.h"


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
