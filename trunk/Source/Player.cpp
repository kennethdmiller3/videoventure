#include "StdAfx.h"
#include "Player.h"
#include "Bullet.h"

// player physics
const float PLAYER_RADIUS = 16;
const float PLAYER_VEL = 200;
const float PLAYER_ACC = 1000;
const float PLAYER_DEC = 50;

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
, input(NULL), axis_x(1, 0), axis_y(0, 1), mDelay(0.0f), mCycle(0)
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
	float cx = (float)(*input)[Input::MOVE_LEFT] - (float)(*input)[Input::MOVE_RIGHT];
	float cy = (float)(*input)[Input::MOVE_UP] - (float)(*input)[Input::MOVE_DOWN];
	float control = std::min(cx*cx + cy*cy, 1.0f);
	float acc = PLAYER_DEC + (PLAYER_ACC - PLAYER_DEC) * control;
	Vector2 dv(cx * PLAYER_VEL - vel.x, cy * PLAYER_VEL - vel.y);
	float it = std::min(aStep * acc / sqrtf(dv.x * dv.x + dv.y * dv.y + 0.0001f), 1.0f);
	vel += dv * it;

	// orient towards mouse cursor
	int mx, my;
	SDL_GetMouseState(&mx, &my);
	axis_y.x = (float)(SCREEN_WIDTH/2 - mx);
	axis_y.y = (float)(SCREEN_HEIGHT/2 - my);
	axis_y *= 1.0f/sqrtf(axis_y.x*axis_y.x+axis_y.y*axis_y.y);
	axis_x.x = axis_y.y;
	axis_x.y = -axis_y.x;

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
