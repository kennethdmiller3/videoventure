#include "StdAfx.h"

#include "Tag.h"

namespace Database
{
	// to do: handle different types
	Typed<Typed<Tag> > tag(0x95f72993 /* "tag" */);

	namespace Loader
	{
		void TagConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			Typed<Tag> &tags = Database::tag.Open(aId);
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				unsigned int aSubId = Hash(child->Value());
				Tag &tag = tags.Open(aSubId);
				child->QueryFloatAttribute("float", &tag.f);
				child->QueryIntAttribute("int", &tag.i);
				tags.Close(aSubId);
			}
			Database::tag.Close(aId);
		}
		Configure tagconfigure(0x95f72993 /* "tag" */, TagConfigure);
	}
}
