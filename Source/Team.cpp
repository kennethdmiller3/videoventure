#include "StdAfx.h"
#include "Team.h"

namespace Database
{
	// team identifier database
	Typed<unsigned int> team(0xa2fd7d0c /* "team" */);

	namespace Loader
	{
		static void TeamConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			unsigned int &team = Database::team.Open(aId);
			team = Hash(element->Attribute("name"));
			Database::team.Close(aId);
		}
		Configure teamconfigure(0xa2fd7d0c /* "team" */, TeamConfigure);
	}
}
