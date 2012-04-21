#include "StdAfx.h"
#include "Damagable.h"
#include "Player.h"

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

			~CriticalItemLoader()
			{
				RemoveConfigure(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
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

			~CriticalItemInitializer()
			{
				RemoveActivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::Activate));
				RemoveDeactivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				Damagable::DeathSignal &signal = Database::deathsignal.Open(aId);
				signal.Connect(this, &CriticalItemInitializer::OnDeath);
				Database::deathsignal.Close(aId);
			}

			void Deactivate(unsigned int aId)
			{
				Damagable::DeathSignal &signal = Database::deathsignal.Open(aId);
				signal.Disconnect(this, &CriticalItemInitializer::OnDeath);
				Database::deathsignal.Close(aId);
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
