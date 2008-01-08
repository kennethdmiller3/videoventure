#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"

namespace Database
{
	Typed<bool> playertemplate(0x4893610a /* "playertemplate" */);
	Typed<Player *> player(0x2c99c300 /* "player" */);

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
				bool &player = Database::playertemplate.Open(aId);
				player = true;
				Database::playertemplate.Close(aId);
			}
		}
		playerloader;
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
				Player *player = new Player(aId);
				Database::player.Put(aId, player);
				Database::controller.Put(aId, player);
			}

			void Deactivate(unsigned int aId)
			{
				if (Player *player = Database::player.Get(aId))
				{
					delete player;
					Database::player.Delete(aId);
					Database::controller.Delete(aId);
				}
			}
		}
		playerinitializer;
	}
}

// Player Constructor
Player::Player(unsigned int aId)
: Controller(aId)
{
}

// Player Destructor
Player::~Player(void)
{
}

// configure
bool Player::Configure(const TiXmlElement *element)
{
	return Controller::Configure(element);
}

// Player Control
void Player::Control(float aStep)
{
	// set inputs
	mMove.x = input[Input::MOVE_HORIZONTAL];
	mMove.y = input[Input::MOVE_VERTICAL];
	mAim.x = input[Input::AIM_HORIZONTAL];
	mAim.y = input[Input::AIM_VERTICAL];
	mFire = input[Input::FIRE_PRIMARY] > 0.0f;
}
