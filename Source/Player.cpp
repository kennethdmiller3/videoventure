#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"
#include "Damagable.h"
#include "Points.h"

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
: mLives(3)
{
}

// player template configure
bool PlayerTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryIntAttribute("lives", &mLives);
	return true;
}


/*
// PLAYER
*/

// player default constructor
Player::Player(void)
: mId(0)
, mAttach(0)
, mLives(0)
, mScore(0)
{
}

// player constructor
Player::Player(const PlayerTemplate &aTemplate, unsigned int aId)
: mId(aId)
, mAttach(0)
, mLives(aTemplate.mLives)
, mScore(0)
{
	// add a kill listener
	Database::Typed<Damagable::KillListener> &listeners = Database::killlistener.Open(mId);
	Damagable::KillListener &listener = listeners.Open(Database::Key(this));
	listener.bind(this, &Player::GotKill);
	listeners.Close(Database::Key(this));
	Database::killlistener.Close(mId);
}

// player destructor
Player::~Player(void)
{
	// remove any kill listener
	Database::Typed<Damagable::KillListener> &listeners = Database::killlistener.Open(mId);
	listeners.Delete(Database::Key(this));
	Database::killlistener.Close(mId);
}

// player attach
void Player::Attach(unsigned int aAttach)
{
	// if attached...
	if (mAttach)
	{
		// detach from existing entity
		Detach();
	}

	// attach to entity
	mAttach = aAttach;

	// set player as owner
	Database::owner.Put(mAttach, mId);

	// add a player controller
	PlayerController *controller = new PlayerController(mAttach);
	Database::playercontroller.Put(mAttach, controller);
	Database::controller.Put(mAttach, controller);
}

// player detach
void Player::Detach(void)
{
	// do nothing if not attached
	if (!mAttach)
		return;

	// remove controller
	PlayerController *controller = Database::playercontroller.Get(mAttach);
	delete controller;
	Database::playercontroller.Delete(mAttach);
	Database::controller.Delete(mAttach);

	// set entity as owner
	Database::owner.Put(mAttach, mAttach);

	// detach from enemy
	mAttach = 0;
}

// player kill notification
void Player::GotKill(unsigned int aId, unsigned int aKillId)
{
	// add points to score
	mScore += Database::points.Get(aKillId);
}


/*
// PLAYER CONTROLLER
*/

// player controller constructor
PlayerController::PlayerController(unsigned int aId)
: Controller(aId)
{
	unsigned int aOwnerId = Database::owner.Get(id);
	if (Player *player = Database::player.Get(aOwnerId))
	{
		player->mAttach = id;
	}
}

// player controller destructor
PlayerController::~PlayerController(void)
{
	unsigned int aOwnerId = Database::owner.Get(id);
	if (Player *player = Database::player.Get(aOwnerId))
	{
		player->mAttach = 0;
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
