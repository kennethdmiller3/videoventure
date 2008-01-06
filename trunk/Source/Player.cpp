#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"

namespace Database
{
	Typed<Player *> player(0x2c99c300 /* "player" */);
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
bool Player::Configure(TiXmlElement *element)
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
