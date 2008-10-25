#include "StdAfx.h"
#include "Player.h"
#include "Entity.h"
#include "Expire.h"
#include "Damagable.h"
#include "Link.h"
#include "Collidable.h"


// game-specific components for Session 5

class PlayerResetTemplate
{
public:
	float mOffset;
};

class PlayerReset : public Updatable
{
public:
	PlayerReset(void);
	PlayerReset(const PlayerResetTemplate &aTemplate, unsigned int aId);
	void Update(float aStep);
	void OnDeath(unsigned int aId, unsigned int aSourceId);

protected:
	void Reset(Player *player);
};

namespace Database
{
	Typed<PlayerResetTemplate> playerresettemplate(0xb683cbdd /* "playerresettemplate" */);
	Typed<PlayerReset *> playerreset(0x006f2a3f /* "playerreset" */);
	Typed<float> enginebasetemplate(0x8e6d1598 /* "enginebasetemplate" */);
	Typed<float> engineaddtemplate(0xbe47c2c0 /* "engineaddtemplate" */);
	Typed<float> setspeed(0x3631419e /* "setspeed" */);
	Typed<bool> criticalitem(0x26de6ef7 /* "criticalitem" */);

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

		class EngineBaseLoader
		{
		public:
			EngineBaseLoader()
			{
				AddConfigure(0xeb9ca706 /* "enginebase" */, Entry(this, &EngineBaseLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				float &enginebase = Database::enginebasetemplate.Open(aId);
				element->QueryFloatAttribute("speed", &enginebase);
				Database::enginebasetemplate.Close(aId);
			}
		}
		enginebaseloader;

		class EngineAddLoader
		{
		public:
			EngineAddLoader()
			{
				AddConfigure(0xc31e166e /* "engineadd" */, Entry(this, &EngineAddLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				float &engineadd = Database::engineaddtemplate.Open(aId);
				element->QueryFloatAttribute("speed", &engineadd);
				Database::engineaddtemplate.Close(aId);
			}
		}
		engineaddloader;

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

		static void ApplySpeed(unsigned int aId, float aSpeed)
		{
			if (Entity *entity = Database::entity.Get(aId))
			{
				Vector2 velocity(entity->GetTransform().Rotate(Vector2(0, aSpeed)));
				entity->SetVelocity(velocity);
				if (Collidable *collidable = Database::collidable.Get(aId))
					collidable->GetBody()->SetLinearVelocity(velocity);
			}
		}

		class EngineBaseInitializer
		{
		public:
			EngineBaseInitializer()
			{
				AddActivate(0x8e6d1598 /* "enginebasetemplate" */, Entry(this, &EngineBaseInitializer::Activate));
				AddDeactivate(0x8e6d1598 /* "enginebasetemplate" */, Entry(this, &EngineBaseInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				float &setspeed = Database::setspeed.Open(aId);
				setspeed = Database::enginebasetemplate.Get(aId);
				ApplySpeed(aId, setspeed);
				Database::setspeed.Close(aId);
			}

			void Deactivate(unsigned int aId)
			{
			}
		}
		enginebaseinitializer;

		class EngineAddInitializer
		{
		public:
			EngineAddInitializer()
			{
				AddActivate(0xbe47c2c0 /* "engineaddtemplate" */, Entry(this, &EngineAddInitializer::Activate));
				AddDeactivate(0xbe47c2c0 /* "engineaddtemplate" */, Entry(this, &EngineAddInitializer::Deactivate));
			}

			void Apply(unsigned int aId, float aDelta)
			{
				float &setspeed = Database::setspeed.Open(aId);
				setspeed += aDelta;
				ApplySpeed(aId, setspeed);
				Database::setspeed.Close(aId);
			}

			void Activate(unsigned int aId)
			{
				// backtrack to the entity containing the base speed
				for (unsigned int id = aId; id != 0; id = Database::backlink.Get(id))
				{
					if (Database::setspeed.Find(id))
					{
						Apply(id, Database::engineaddtemplate.Get(aId));
						break;
					}
				}
			}

			void Deactivate(unsigned int aId)
			{
				// backtrack to the entity containing the base speed
				for (unsigned int id = aId; id != 0; id = Database::backlink.Get(id))
				{
					if (Database::setspeed.Find(id))
					{
						Apply(id, -Database::engineaddtemplate.Get(aId));
						break;
					}
				}
			}
		}
		engineaddinitializer;

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
