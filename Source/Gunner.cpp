#include "StdAfx.h"
#include "Gunner.h"
#include "Bullet.h"

// gunner physics
const float GUNNER_RADIUS = 5;

// gunner bullet physics
const float GUNNER_BULLET_SPEED = 800;

// gunner offset (keyed by gunner index)
const Vector2 GUNNER_OFFSET[2] =
{
	Vector2(-24, -16),
	Vector2(24, -16),
};

// aim direction (keyed by gunner index)
const float GUNNER_AIM_ANGLE = 10*(float)M_PI/180;
const Vector2 GUNNER_AIM_DIR[2] =
{
	Vector2(sinf(-GUNNER_AIM_ANGLE), cosf(-GUNNER_AIM_ANGLE)),
	Vector2(sinf(GUNNER_AIM_ANGLE), cosf(GUNNER_AIM_ANGLE)),
};

// bullet direction (keyed by shot index)
const float GUNNER_BULLET_ANGLE = 5*(float)M_PI/180;
const Vector2 GUNNER_BULLET_DIR[2] =
{
	Vector2(sinf(-GUNNER_BULLET_ANGLE), cosf(-GUNNER_BULLET_ANGLE)),
	Vector2(sinf(GUNNER_BULLET_ANGLE), cosf(GUNNER_BULLET_ANGLE)),
};


// Gunner Constructor
Gunner::Gunner(void)
: Entity(), Controllable(), Simulatable(), Renderable()
, input(NULL), player(NULL), axis_x(1, 0), axis_y(0, 1), mDelay(0.0f), mPhase(-1), mCycle(0)
{
}

// Gunner Destructor
Gunner::~Gunner(void)
{
}

// Gunner Control
void Gunner::Control(float aStep)
{
	if (!input)
		return;

	if ((*input)[Input::FIRE_PRIMARY])
	{
		if (mDelay <= 0.0f)
		{
			if (mCycle == mPhase)
			{
				for (int i = 0; i < 2; i++)
				{
					const Vector2 &d = GUNNER_BULLET_DIR[i];
					Bullet *bullet = new Bullet();
					bullet->SetPosition(pos + axis_x * GUNNER_RADIUS * d.x + axis_y * GUNNER_RADIUS * d.y);
					bullet->SetVelocity(axis_x * GUNNER_BULLET_SPEED * d.x + axis_y * GUNNER_BULLET_SPEED * d.y);
				}
			}

			mDelay += 0.125f;
			mCycle = 1 - mCycle;
		}
	}
}

// Gunner Simulate
void Gunner::Simulate(float aStep)
{
	if (!player)
		return;

	// snap to player position
	const Vector2 &p = GUNNER_OFFSET[mPhase];
	pos = player->GetPosition() + player->GetAxisX() * p.x + player->GetAxisY() * p.y;

	// snap to player orientation
	const Vector2 &d = GUNNER_AIM_DIR[mPhase];
	axis_x = player->GetAxisX() * d.y - player->GetAxisY() * d.x;
	axis_y = player->GetAxisX() * d.x + player->GetAxisY() * d.y;

	// update fire delay
	mDelay -= aStep;
	if (mDelay < 0.0f)
		mDelay = 0.0f;
}

// Gunner Render
void Gunner::Render()
{
	// push a transform
	glPushMatrix();

	// load matrix
	float m[16] =
	{
		axis_y.y, -axis_y.x, 0, 0,
		axis_y.x, axis_y.y, 0, 0,
		0, 0, 1, 0,
		pos.x, pos.y, 0, 1
	};
	glMultMatrixf( m );

	// call draw list
	glCallList(mDraw);

	// reset the transform
	glPopMatrix();
}
