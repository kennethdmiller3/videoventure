#include "StdAfx.h"

#include "PlayerReset.h"
#include "Player.h"
#include "Entity.h"
#include "Expire.h"

namespace Database
{
	Typed<PlayerResetTemplate> playerresettemplate(0xb683cbdd /* "playerresettemplate" */);
	Typed<PlayerReset *> playerreset(0x006f2a3f /* "playerreset" */);

	namespace Loader
	{
		class PlayerResetLoader
		{
		public:
			PlayerResetLoader()
			{
				AddConfigure(0x006f2a3f /* "playerreset" */, Entry(this, &PlayerResetLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				PlayerResetTemplate &playerreset = Database::playerresettemplate.Open(aId);
				element->QueryFloatAttribute("offset", &playerreset.mOffset);
				Database::playerresettemplate.Close(aId);
			}
		}
		playerresetloader;
	}

	namespace Initializer
	{
		class PlayerResetInitializer
		{
		public:
			PlayerResetInitializer()
			{
				AddActivate(0xb683cbdd /* "playerresettemplate" */, Entry(this, &PlayerResetInitializer::Activate));
				AddDeactivate(0xb683cbdd /* "playerresettemplate" */, Entry(this, &PlayerResetInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const PlayerResetTemplate &playerresettemplate = Database::playerresettemplate.Get(aId);
				PlayerReset *playerreset = new PlayerReset(playerresettemplate, aId);
				Database::playerreset.Put(aId, playerreset);
			}

			void Deactivate(unsigned int aId)
			{
				if (PlayerReset *playerreset = Database::playerreset.Get(aId))
				{
					delete playerreset;
					Database::playerreset.Delete(aId);
				}
			}
		}
		playerresetinitializer;
	}
}

PlayerReset::PlayerReset(void)
	: Updatable(0)
{
	SetAction(Action(this, &PlayerReset::Update));
}

PlayerReset::PlayerReset(const PlayerResetTemplate &aTemplate, unsigned int aId)
	: Updatable(aId)
{
	SetAction(Action(this, &PlayerReset::Update));
	Activate();

	Database::Typed<Damagable::DeathListener> &deathlisteners = Database::deathlistener.Open(mId);
	deathlisteners.Put(reinterpret_cast<unsigned int>(this), Damagable::DeathListener(this, &PlayerReset::OnDeath));
	Database::deathlistener.Close(mId);
}

void PlayerReset::Update(float aStep)
{
	// for each player...
	for (Database::Typed<Player *>::Iterator itor(&Database::player); itor.IsValid(); ++itor)
	{
		// get the player
		Player *player = itor.GetValue();

		// if the player has an attached entity...
		if (Entity *playerentity = Database::entity.Get(player->mAttach))
		{
			// get the reference entity
			if (Entity *entity = Database::entity.Get(mId))
			{
				// if the player entity moves past the threshold position...
				if (playerentity->GetPosition().y > entity->GetPosition().y + Database::playerresettemplate.Get(mId).mOffset)
				{
					// reset the player
					Reset(player);
				}
			}
		}
	}
}

void PlayerReset::OnDeath(unsigned int aId, unsigned int aSourceId)
{
	// for each player...
	for (Database::Typed<Player *>::Iterator itor(&Database::player); itor.IsValid(); ++itor)
	{
		// get the player
		Player *player = itor.GetValue();

		// reset the player
		Reset(player);
	}
}

void PlayerReset::Reset(Player *player)
{
	// set to expire
	ExpireTemplate &expiretemplate = Database::expiretemplate.Open(player->mAttach);
	expiretemplate.mTime = Database::playertemplate.Get(player->mAttach).mCycle;
	expiretemplate.mSpawn = 0;
	expiretemplate.mSwitch = 0;
	Database::expiretemplate.Close(player->mAttach);
	Database::expire.Put(player->mAttach, new Expire(expiretemplate, player->mAttach));

	// reset the player without losing a life
	++player->mLives;
	player->Detach(player->mAttach);
	player->Activate();
}
