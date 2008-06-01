#include "StdAfx.h"
#include "Player.h"
#include "Entity.h"
#include "Bullet.h"
#include "Damagable.h"
#include "Capturable.h"
#include "Points.h"
#include "Sound.h"

namespace Database
{
	Typed<PlayerTemplate> playertemplate(0x4893610a /* "playertemplate" */);
	Typed<Player *> player(0x2c99c300 /* "player" */);
	Typed<bool> playercontrollertemplate(0xec81fd12 /* "playercontrollertemplate" */);
	Typed<PlayerController *> playercontroller(0x7a57caa8 /* "playercontroller" */);

	namespace Loader
	{
		class PlayerLoader
		{
		public:
			PlayerLoader()
			{
				AddConfigure(0x2c99c300 /* "player" */, Entry(this, &PlayerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				PlayerTemplate &player = Database::playertemplate.Open(aId);
				player.Configure(element, aId);
				Database::playertemplate.Close(aId);
			}
		}
		playerloader;

		class PlayerControllerLoader
		{
		public:
			PlayerControllerLoader()
			{
				AddConfigure(0x7a57caa8 /* "playercontroller" */, Entry(this, &PlayerControllerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				bool &playercontroller = Database::playercontrollertemplate.Open(aId);
				playercontroller = true;
				Database::playercontrollertemplate.Close(aId);
			}
		}
		playercontrollerloader;
	}

	namespace Initializer
	{
		class PlayerInitializer
		{
		public:
			PlayerInitializer()
			{
				AddActivate(0x4893610a /* "playertemplate" */, Entry(this, &PlayerInitializer::Activate));
				AddDeactivate(0x4893610a /* "playertemplate" */, Entry(this, &PlayerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const PlayerTemplate &playertemplate = Database::playertemplate.Get(aId);
				Player *player = new Player(playertemplate, aId);
				Database::player.Put(aId, player);
				player->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Player *player = Database::player.Get(aId))
				{
					delete player;
					Database::player.Delete(aId);
				}
			}
		}
		playerinitializer;

		class PlayerControllerInitializer
		{
		public:
			PlayerControllerInitializer()
			{
				AddActivate(0xec81fd12 /* "playercontrollertemplate" */, Entry(this, &PlayerControllerInitializer::Activate));
				AddDeactivate(0xec81fd12 /* "playercontrollertemplate" */, Entry(this, &PlayerControllerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				PlayerController *playercontroller = new PlayerController(aId);
				Database::playercontroller.Put(aId, playercontroller);
				Database::controller.Put(aId, playercontroller);

				playercontroller->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (PlayerController *playercontroller = Database::playercontroller.Get(aId))
				{
					delete playercontroller;
					Database::playercontroller.Delete(aId);
					Database::controller.Delete(aId);
				}
			}
		}
		playercontrollerinitializer;
	}
}

/*
// PLAYER TEMPLATE
*/

// player template constructor
PlayerTemplate::PlayerTemplate(void)
: mSpawn(0)
, mLives(INT_MAX)
, mExtra(INT_MAX)
{
}

// player template configure
bool PlayerTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	if (const char *spawn = element->Attribute("name"))
		mSpawn = Hash(spawn);
	element->QueryIntAttribute("lives", &mLives);
	element->QueryIntAttribute("extra", &mExtra);
	return true;
}


/*
// PLAYER
*/

// player default constructor
Player::Player(void)
: Updatable(0)
, mAttach(0)
, mLives(0)
, mScore(0)
{
}

// player constructor
Player::Player(const PlayerTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mAttach(0)
, mLives(aTemplate.mLives)
, mScore(0)
{
	{
		// add a kill listener
		Database::Typed<Damagable::KillListener> &listeners = Database::killlistener.Open(mId);
		Damagable::KillListener &listener = listeners.Open(Database::Key(this));
		listener.bind(this, &Player::GotKill);
		listeners.Close(Database::Key(this));
		Database::killlistener.Close(mId);
	}

	{
		// add a capture listener
		Database::Typed<Capturable::CaptureListener> &listeners = Database::capturelistener.Open(mId);
		Capturable::CaptureListener &listener = listeners.Open(Database::Key(this));
		listener.bind(this, &Player::GotKill);
		listeners.Close(Database::Key(this));
		Database::capturelistener.Close(mId);
	}
}

// player destructor
Player::~Player(void)
{
	{
		// remove any capture listener
		Database::Typed<Capturable::CaptureListener> &listeners = Database::capturelistener.Open(mId);
		listeners.Delete(Database::Key(this));
		Database::capturelistener.Close(mId);
	}

	{
		// remove any kill listener
		Database::Typed<Damagable::KillListener> &listeners = Database::killlistener.Open(mId);
		listeners.Delete(Database::Key(this));
		Database::killlistener.Close(mId);
	}
}

// player update
void Player::Update(float aStep)
{
	// if attached to an entity...
	if (mAttach)
	{
		// if the entity is still alive...
		if (Database::entity.Get(mAttach))
		{
			// wait
			return;
		}
		else
		{
			// detach
			Detach();
		}
	}

	// spawn a new entity
	Spawn();
}

// player attach
void Player::Attach(unsigned int aAttach)
{
	// attach to entity
	mAttach = aAttach;

	// add a death listener
	Database::Typed<Damagable::DeathListener> &listeners = Database::deathlistener.Open(mAttach);
	Damagable::DeathListener &listener = listeners.Open(Database::Key(this));
	listener.bind(this, &Player::OnDeath);
	listeners.Close(Database::Key(this));
	Database::deathlistener.Close(mAttach);
}

// player detach
void Player::Detach(void)
{
	// do nothing if not attached
	if (!mAttach)
		return;

	// remove any death listener
	Database::Typed<Damagable::DeathListener> &listeners = Database::deathlistener.Open(mAttach);
	listeners.Delete(Database::Key(this));
	Database::deathlistener.Close(mAttach);

	// detach from enemy
	mAttach = 0;
}

// player spawn
void Player::Spawn(void)
{
	// get the spawner entity
	Entity *entity = Database::entity.Get(mId);
	if (entity)
	{
		// use a life
		if (mLives < INT_MAX)
			--mLives;

		// get the player template
		const PlayerTemplate &playertemplate = Database::playertemplate.Get(mId);

		// instantiate the spawn entity
		// TO DO: use a named spawn point
		Database::Instantiate(playertemplate.mSpawn, mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());

		// done for now
		Deactivate();
	}
}

// player death notification
void Player::OnDeath(unsigned int aId, unsigned int aSourceId)
{
	// if there are lives left...
	if (mLives > 0)
	{
		Activate();
	}
}

// player kill notification
void Player::GotKill(unsigned int aId, unsigned int aKillId)
{
	// get point value
	int aValue = Database::points.Get(aKillId);

	// get the player template
	const PlayerTemplate &playertemplate = Database::playertemplate.Get(mId);

	// if extra lives enabled...
	if (playertemplate.mExtra > 0)
	{
		// extra lives
		int extra = ((mScore + aValue) / playertemplate.mExtra) - (mScore / playertemplate.mExtra);
		if (extra)
		{
			// add any extra lives
			mLives += extra;

			// trigger sound cue
			PlaySound(mAttach, 0x62b13a2b /* "extralife" */);
		}
	}

	// add value to score
	mScore += aValue;

}


/*
// PLAYER CONTROLLER
*/

// player controller constructor
PlayerController::PlayerController(unsigned int aId)
: Controller(aId)
{
	unsigned int aOwnerId = Database::owner.Get(mId);
	if (Player *player = Database::player.Get(aOwnerId))
	{
		player->Attach(mId);
	}
}

// player controller destructor
PlayerController::~PlayerController(void)
{
	unsigned int aOwnerId = Database::owner.Get(mId);
	if (Player *player = Database::player.Get(aOwnerId))
	{
		player->Detach();
	}

}

// player controller ontrol
void PlayerController::Control(float aStep)
{
	// set inputs
	// TO DO: support multiple players
	mMove.x = input[Input::MOVE_HORIZONTAL];
	mMove.y = input[Input::MOVE_VERTICAL];
	mAim.x = input[Input::AIM_HORIZONTAL];
	mAim.y = input[Input::AIM_VERTICAL];
	mFire[0] = input[Input::FIRE_PRIMARY] > 0.0f;
	mFire[1] = input[Input::FIRE_SECONDARY] > 0.0f;
}
