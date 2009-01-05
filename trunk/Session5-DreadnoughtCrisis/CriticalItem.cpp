#include "StdAfx.h"
#include "Damagable.h"

namespace Database
{
	Typed<bool> criticalitem(0x26de6ef7 /* "criticalitem" */);

	namespace Loader
	{
		class CriticalItemLoader
		{
		public:
			CriticalItemLoader()
			{
				AddConfigure(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				bool &criticalitem = Database::criticalitem.Open(aId);
				int critical = criticalitem;
				element->QueryIntAttribute("value", &critical);
				criticalitem = critical != 0;
				Database::criticalitem.Close(aId);
			}
		}
		criticalitemloader;
	}

	namespace Initializer
	{
		class CriticalItemInitializer
		{
		public:
			CriticalItemInitializer()
			{
				AddActivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::Activate));
				AddDeactivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				Database::Typed<Damagable::DeathListener> &deathlisteners = Database::deathlistener.Open(aId);
				deathlisteners.Put(0x26de6ef7 /* "criticalitem" */, Damagable::DeathListener(this, &CriticalItemInitializer::OnDeath));
				Database::deathlistener.Close(aId);
			}

			void Deactivate(unsigned int aId)
			{
				// 
				Database::Typed<Damagable::DeathListener> &deathlisteners = Database::deathlistener.Open(aId);
				deathlisteners.Delete(0x26de6ef7 /* "criticalitem" */);
				Database::deathlistener.Close(aId);
			}

			void OnDeath(unsigned int aId, unsigned int aSourceId)
			{
				// for each player...
				for (Database::Typed<Player *>::Iterator itor(&Database::player); itor.IsValid(); ++itor)
				{
					Player *player = itor.GetValue();

					Database::Delete(player->mAttach);
					player->Detach(player->mAttach);
					player->mLives = 0;
					player->Activate();
				}
			}
		}
		criticaliteminitializer;
	}
}
