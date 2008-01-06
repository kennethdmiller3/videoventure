#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"

namespace Database
{
	Typed<Player *> player("player");
}

// Player Constructor
Player::Player(unsigned int aId, unsigned int aParentId)
: Controllable(aId)
, Simulatable(aId)
, mMaxVeloc(200)
, mMaxAccel(1000)
, mFriction(50)
, mMaxOmega(10)
, mMove(0, 0)
, mAim(0, 0)
{
}

// Player Destructor
Player::~Player(void)
{
}

// configure
bool Player::Configure(TiXmlElement *element)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x2c99c300 /* "player" */:
		{
			element->QueryFloatAttribute("maxveloc", &mMaxVeloc);
			element->QueryFloatAttribute("maxaccel", &mMaxAccel);
			element->QueryFloatAttribute("friction", &mFriction);
			element->QueryFloatAttribute("maxomega", &mMaxOmega);
		}
		return true;

	default:
		return Controllable::Configure(element) || Simulatable::Configure(element);
	}
}

// Player Control
void Player::Control(float aStep)
{
	// set inputs
	mMove.x = input[Input::MOVE_HORIZONTAL];
	mMove.y = input[Input::MOVE_VERTICAL];
	mAim.x = input[Input::AIM_HORIZONTAL];
	mAim.y = input[Input::AIM_VERTICAL];
}

// Player Simulate
void Player::Simulate(float aStep)
{
	// get entity
	Entity *entity = Database::entity.Get(Simulatable::id);

	// get player collidable
	const Collidable *collidable = Database::collidable.Get(Simulatable::id);
	b2Body *body = collidable->GetBody();

	// apply thrust
	{
		float control = std::min(mMove.LengthSq(), 1.0f);
		float acc = mFriction + (mMaxAccel - mFriction) * control;
		Vector2 dv(mMove * mMaxVeloc - entity->GetVelocity());
		float it = std::min(acc * aStep / sqrtf(dv.LengthSq() + 0.0001f), 1.0f);
		Vector2 new_thrust(dv * it * body->GetMass());
		body->ApplyImpulse(b2Vec2(new_thrust.x, new_thrust.y), body->GetCenterPosition());
	}

	// apply steering
	{
		float control = std::min(16.0f * mAim.LengthSq(), 1.0f);
		float aim_angle = -atan2f(mAim.x, mAim.y) - entity->GetAngle();
		if (aim_angle > float(M_PI))
			aim_angle -= 2.0f*float(M_PI);
		else if (aim_angle < -float(M_PI))
			aim_angle += 2.0f*float(M_PI);
		float new_omega = std::min(std::max(aim_angle / aStep, -mMaxOmega * control), mMaxOmega * control);
		body->SetAngularVelocity(new_omega);
	}
}
