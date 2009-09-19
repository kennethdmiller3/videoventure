#include "StdAfx.h"

#include "Tag.h"

namespace Database
{
	// to do: handle different types
	Typed<Typed<Tag> > tag(0x95f72993 /* "tag" */);

	namespace Loader
	{
		class TagLoader
		{
		public:
			TagLoader()
			{
				AddConfigure(0x95f72993 /* "tag" */, Entry(this, &TagLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Typed<Tag> &tags = Database::tag.Open(aId);
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					unsigned int aSubId = Hash(child->Value());
					Tag &tag = tags.Open(aSubId);
					child->QueryFloatAttribute("float", &tag.f);
					child->QueryIntAttribute("int", &tag.i);
					tags.Close(aSubId);
				}
				Database::tag.Close(aId);
			}
		}
		tagloader;
	}
}
