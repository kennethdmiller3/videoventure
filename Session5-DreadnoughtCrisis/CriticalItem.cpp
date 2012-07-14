#include "StdAfx.h"
#include "Damagable.h"
#include "Player.h"

namespace Database
{
	Typed<bool> criticalitem(0x26de6ef7 /* "criticalitem" */);

	namespace Loader
	{
		static void CriticalItemConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			bool &criticalitem = Database::criticalitem.Open(aId);
			element->QueryBoolAttribute("value", &criticalitem);
			Database::criticalitem.Close(aId);
		}
		Configure criticalitemconfigure(0x26de6ef7 /* "criticalitem" */, CriticalItemConfigure);
	}

	namespace Initializer
	{
		static void CriticalItemOnDeath(unsigned int aId, unsigned int aSourceId)
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

		static void CriticalItemActivate(unsigned int aId)
		{
			Damagable::DeathSignal &signal = Database::deathsignal.Open(aId);
			signal.Connect(Damagable::DeathSignal::Slot(CriticalItemOnDeath));
			Database::deathsignal.Close(aId);
		}
		Activate criticalitemactivate(0x26de6ef7 /* "criticalitem" */, CriticalItemActivate);

		static void CriticalItemDeactivate(unsigned int aId)
		{
			Damagable::DeathSignal &signal = Database::deathsignal.Open(aId);
			signal.Disconnect(Damagable::DeathSignal::Slot(CriticalItemOnDeath));
			Database::deathsignal.Close(aId);
		}
		Deactivate criticalitemdeactivate(0x26de6ef7 /* "criticalitem" */, CriticalItemDeactivate);
	}
}
