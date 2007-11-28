#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"

// player physics
const float PLAYER_RADIUS = 16;
const float PLAYER_VEL = 200;
const float PLAYER_ACC = 1000;
const float PLAYER_DEC = 50;
const float PLAYER_OMEGA = 3.0f * float(M_PI);

// player bullet physics
const float PLAYER_BULLET_SPEED = 800;

// edge boundaries
const float EDGE_LEFT = ARENA_X_MIN + PLAYER_RADIUS;
const float EDGE_RIGHT = ARENA_X_MAX - PLAYER_RADIUS;
const float EDGE_TOP = ARENA_Y_MIN + PLAYER_RADIUS;
const float EDGE_BOTTOM = ARENA_Y_MAX - PLAYER_RADIUS;

// Player Constructor
Player::Player(void)
: Entity(), Controllable(), Simulatable(), Collidable(), Renderable()
, input(NULL), axis_x(1, 0), axis_y(0, 1), omega(0.0f), mDelay(0.0f), mCycle(0)
{
}

// Player Destructor
Player::~Player(void)
{
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
		float acc = PLAYER_DEC + (PLAYER_ACC - PLAYER_DEC) * control;
		Vector2 dv(move * PLAYER_VEL - vel);
		float it = std::min(aStep * acc / sqrtf(dv.LengthSq() + 0.0001f), 1.0f);
		vel += dv * it;
	}

	// apply steering
	{
		Vector2 aim((*input)[Input::AIM_HORIZONTAL], (*input)[Input::AIM_VERTICAL]);
		float control = std::min(16.0f * aim.LengthSq(), 1.0f);
		float cur_angle = atan2f(axis_y.x, axis_y.y);
		float aim_angle = atan2f(aim.x, aim.y);
		if (aim_angle > cur_angle+float(M_PI))
			aim_angle -= 2.0f*float(M_PI);
		else if (aim_angle < cur_angle-float(M_PI))
			aim_angle += 2.0f*float(M_PI);
		float new_angle = cur_angle + std::min(std::max(aim_angle - cur_angle, -PLAYER_OMEGA * control * aStep), PLAYER_OMEGA * control * aStep);
		axis_y.x = sinf(new_angle);
		axis_y.y = cosf(new_angle);
		axis_x.x = axis_y.y;
		axis_x.y = -axis_y.x;
	}

	if ((*input)[Input::FIRE_PRIMARY])
	{
		if (mDelay <= 0.0f)
		{
			Bullet *bullet;
			bullet = new Bullet();
			bullet->SetPosition(pos - axis_x * 6);
			bullet->SetVelocity(axis_y * PLAYER_BULLET_SPEED);
			bullet = new Bullet();
			bullet->SetPosition(pos + axis_x * 6);
			bullet->SetVelocity(axis_y * PLAYER_BULLET_SPEED);

			mDelay += 0.125f;
		}
	}
}

// Player Simulate
void Player::Simulate(float aStep)
{
	// apply velocity
	pos += vel * aStep;

	// bounce off boundary
	if( pos.x < EDGE_LEFT )
	{
		pos.x = EDGE_LEFT;
		vel.x *= -0.5f;
	}
	else if( pos.x > EDGE_RIGHT )
	{
		pos.x = EDGE_RIGHT;
		vel.x *= -0.5f;
	}
	if( pos.y < EDGE_TOP )
	{
		pos.y = EDGE_TOP;
		vel.y *= -0.5f;
	}
	else if( pos.y > EDGE_BOTTOM)
	{
		pos.y = EDGE_BOTTOM;
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
		axis_x.x, axis_x.y, 0, 0,
		axis_y.x, axis_y.y, 0, 0,
		0, 0, 1, 0,
		pos.x, pos.y, 0, 1
	};
	glMultMatrixf( m );

	// call the draw list
	glCallList(mDraw);

	// reset the transform
	glPopMatrix();
}
