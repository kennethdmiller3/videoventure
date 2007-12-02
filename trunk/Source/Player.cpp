#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"

// player bullet physics
const float PLAYER_BULLET_SPEED = 800;

// Player Constructor
Player::Player(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Controllable()
, Simulatable()
, Collidable(Database::collidabletemplate.Get(aParentId))
, Renderable(Database::renderabletemplate.Get(aParentId))
, input(NULL)
, mMaxVeloc(200), mMaxAccel(1000), mFriction(50), mMaxOmega(10)
, omega(0.0f), mDelay(0.0f), mCycle(0)
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
		return Entity::Configure(element) || Controllable::Configure(element) || Simulatable::Configure(element) || Renderable::Configure(element);
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
		float it = std::min(aStep * acc / sqrtf(dv.LengthSq() + 0.0001f), 1.0f);
		vel += dv * it;
	}

	// apply steering
	{
		Vector2 aim((*input)[Input::AIM_HORIZONTAL], (*input)[Input::AIM_VERTICAL]);
		float control = std::min(16.0f * aim.LengthSq(), 1.0f);
		float cur_angle = atan2f(transform.y.x, transform.y.y);
		float aim_angle = atan2f(aim.x, aim.y);
		if (aim_angle > cur_angle+float(M_PI))
			aim_angle -= 2.0f*float(M_PI);
		else if (aim_angle < cur_angle-float(M_PI))
			aim_angle += 2.0f*float(M_PI);
		float new_angle = cur_angle + std::min(std::max(aim_angle - cur_angle, -mMaxOmega * control * aStep), mMaxOmega * control * aStep);
		transform.y.x = sinf(new_angle);
		transform.y.y = cosf(new_angle);
		transform.x.x = transform.y.y;
		transform.x.y = -transform.y.x;
	}

	if ((*input)[Input::FIRE_PRIMARY])
	{
		if (mDelay <= 0.0f)
		{
			Bullet *bullet;
			bullet = new Bullet(0, 0xd85669f0 /* "playerbullet" */);
			bullet->SetPosition(transform.Transform(Vector2(-6, 0)));
			bullet->SetVelocity(transform.y * PLAYER_BULLET_SPEED);
			bullet = new Bullet(0, 0xd85669f0 /* "playerbullet" */);
			bullet->SetPosition(transform.Transform(Vector2(6, 0)));
			bullet->SetVelocity(transform.y * PLAYER_BULLET_SPEED);

			mDelay += 0.125f;
		}
	}
}

// Player Simulate
void Player::Simulate(float aStep)
{
	// apply velocity
	transform.p += vel * aStep;

	// bounce off boundary
	if( transform.p.x < ARENA_X_MIN + Collidable::size.x )
	{
		transform.p.x = ARENA_X_MIN + Collidable::size.x;
		vel.x *= -0.5f;
	}
	else if( transform.p.x > ARENA_X_MAX - Collidable::size.x )
	{
		transform.p.x = ARENA_X_MAX - Collidable::size.x;
		vel.x *= -0.5f;
	}
	if( transform.p.y < ARENA_Y_MIN + Collidable::size.y )
	{
		transform.p.y = ARENA_Y_MIN + Collidable::size.y;
		vel.y *= -0.5f;
	}
	else if( transform.p.y > ARENA_Y_MAX - Collidable::size.y)
	{
		transform.p.y = ARENA_Y_MAX - Collidable::size.y;
		vel.y *= -0.5f;
	}

	// update fire delay
	mDelay -= aStep;
	if (mDelay < 0.0f)
		mDelay = 0.0f;
}

// Player Collide
void Player::Collide(float aStep, Collidable &aRecipient)
{
}

// Player Render
void Player::Render()
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
