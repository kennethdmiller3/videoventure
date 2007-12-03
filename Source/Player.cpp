#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"

// player bullet physics
const float PLAYER_BULLET_SPEED = 800;
const Vector2 PLAYER_BULLET_POS[2] =
{
	Vector2(-4, 0),
	Vector2(4, 0)
};

// Player Constructor
Player::Player(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Controllable()
, Simulatable()
, Collidable(Database::collidabletemplate.Get(aParentId))
, Renderable(Database::renderabletemplate.Get(aParentId))
, input(NULL)
, mMaxVeloc(200), mMaxAccel(1000), mFriction(50), mMaxOmega(10)
, mDelay(0.0f), mCycle(0)
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
		return Entity::Configure(element) || Controllable::Configure(element) || Simulatable::Configure(element) || Collidable::Configure(element) || Renderable::Configure(element);
	}
}

// Player Control
void Player::Control(float aStep)
{
	if (!input)
		return;

	// apply thrust
	{
		Vector2 move((*input)[Input::MOVE_HORIZONTAL], (*input)[Input::MOVE_VERTICAL]);
		float control = std::min(move.LengthSq(), 1.0f);
		float acc = mFriction + (mMaxAccel - mFriction) * control;
		Vector2 dv(move * mMaxVeloc - vel);
		float it = std::min(acc * aStep / sqrtf(dv.LengthSq() + 0.0001f), 1.0f);
		Vector2 new_thrust(dv * it * body->GetMass());
		body->ApplyImpulse(b2Vec2(new_thrust.x, new_thrust.y), body->GetCenterPosition());
	}

	// apply steering
	{
		Vector2 aim((*input)[Input::AIM_HORIZONTAL], (*input)[Input::AIM_VERTICAL]);
		float control = std::min(16.0f * aim.LengthSq(), 1.0f);
		float aim_angle = -atan2f(aim.x, aim.y) - angle_1;
		if (aim_angle > float(M_PI))
			aim_angle -= 2.0f*float(M_PI);
		else if (aim_angle < -float(M_PI))
			aim_angle += 2.0f*float(M_PI);
		float new_omega = std::min(std::max(aim_angle / aStep, -mMaxOmega * control), mMaxOmega * control);
		body->SetAngularVelocity(new_omega);
	}

	// advance fire timer
	mDelay -= aStep * mCycle;

	// if ready to fire...
	if (mDelay <= 0.0f)
	{
		// if triggered...
		if ((*input)[Input::FIRE_PRIMARY])
		{
			// for each shot...
			for (int i = 0; i < 2; i++)
			{
				const Vector2 d = PLAYER_BULLET_POS[i];
				Bullet *bullet = Bullet::pool.construct(0, 0xd85669f0 /* "playerbullet" */);
				bullet->SetTransform(angle_1, posit_1);
				bullet->SetPosition(bullet->GetTransform().Transform(d));
				bullet->SetVelocity(bullet->GetTransform().y * PLAYER_BULLET_SPEED);
				bullet->Init();
				bullet->AddToWorld();
			}

			// update weapon delay
			mDelay += 0.25f;
		}
		else
		{
			// clamp fire delay
			mDelay = 0.0f;
		}
	}
}

// Player Simulate
void Player::Simulate(float aStep)
{
	// update fire delay
	mDelay -= aStep;
	if (mDelay < 0.0f)
		mDelay = 0.0f;
}

// Player Collide
void Player::Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount)
{
}

// Player Render
void Player::Render(const Matrix2 &transform)
{
	// push a transform
	glPushMatrix();

	// load matrix
	float m[16] =
	{
		transform.x.x, transform.x.y, 0, 0,
		transform.y.x, transform.y.y, 0, 0,
		0, 0, 1, 0,
		transform.p.x, transform.p.y, 0, 1
	};
	glMultMatrixf( m );

	// call the draw list
	glCallList(mDraw);

	// reset the transform
	glPopMatrix();
}
