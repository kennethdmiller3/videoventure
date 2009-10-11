#include "StdAfx.h"
#include "Team.h"

namespace Database
{
	// team identifier database
	GAME_API Typed<unsigned int> team(0xa2fd7d0c /* "team" */);

	namespace Loader
	{
		class TeamLoader
		{
		public:
			TeamLoader()
			{
				AddConfigure(0xa2fd7d0c /* "team" */, Entry(this, &TeamLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				unsigned int &team = Database::team.Open(aId);
				team = Hash(element->Attribute("name"));
				Database::team.Close(aId);
			}
		}
		teamloader;
	}
}
