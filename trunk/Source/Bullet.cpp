#include "StdAfx.h"
#include "Bullet.h"
#include "Explosion.h"

// bullet attributes
const float BULLET_RADIUS = 3;
const float BULLET_LIFE = 1.0f;

// bullet geometry (unscaled)
const float BULLET_ANGLE = (float)M_PI/6;
const Vector2 BULLET_VERTICES[] =
{
	Vector2(cosf( 0*BULLET_ANGLE), sinf( 0*BULLET_ANGLE)),
	Vector2(cosf( 1*BULLET_ANGLE), sinf( 1*BULLET_ANGLE)),
	Vector2(cosf(11*BULLET_ANGLE), sinf(11*BULLET_ANGLE)),
	Vector2(cosf( 2*BULLET_ANGLE), sinf( 2*BULLET_ANGLE)),
	Vector2(cosf(10*BULLET_ANGLE), sinf(10*BULLET_ANGLE)),
	Vector2(cosf( 3*BULLET_ANGLE), sinf( 3*BULLET_ANGLE)),
	Vector2(cosf( 9*BULLET_ANGLE), sinf( 9*BULLET_ANGLE)),
	Vector2(cosf( 4*BULLET_ANGLE), sinf( 4*BULLET_ANGLE)),
	Vector2(cosf( 8*BULLET_ANGLE), sinf( 8*BULLET_ANGLE)),
	Vector2(cosf( 5*BULLET_ANGLE), sinf( 5*BULLET_ANGLE)),
	Vector2(cosf( 7*BULLET_ANGLE), sinf( 7*BULLET_ANGLE)),
	Vector2(cosf( 6*BULLET_ANGLE), sinf( 6*BULLET_ANGLE)),
};

// bullet core geometry
const float BULLET_CORE_RADIUS = 3;
const float BULLET_CORE_COLOR[4] = { 0.8f, 0.9f, 1.0f, 1.0f };

// bullet halo geometry
const float BULLET_HALO_RADIUS = 6;
const float BULLET_HALO_COLOR[4] = { 0.0f, 0.5f, 1.0f, 0.25f };

// bullet tail geometry
const float BULLET_TAIL_LENGTH = 0.0625f;
const float BULLET_TAIL_COLOR[2][4] =
{
	{ 1.0f, 1.0f, 1.0f, 0.5f },
	{ 0.0f, 0.0f, 1.0f, 0.0f }
};

Bullet::Bullet(void)
: Entity(), Simulatable(), Collidable(COLLISION_LAYER_PLAYER_BULLET), Renderable()
, mLife(BULLET_LIFE)
{
	Collidable::type = Collidable::TYPE_CIRCLE;
	Collidable::size.x = BULLET_RADIUS;
	Collidable::size.y = BULLET_RADIUS;

	// start a draw list
	mDraw = glGenLists(1);
	glNewList(mDraw, GL_COMPILE);

	// draw core
	glColor4fv( BULLET_CORE_COLOR );
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < SDL_arraysize(BULLET_VERTICES); ++i)
	{
		const Vector2 &p = BULLET_VERTICES[i];
		glVertex2f(BULLET_CORE_RADIUS*p.x, BULLET_CORE_RADIUS*p.y);
	}
	glEnd();

	// draw halo
	glColor4fv( BULLET_HALO_COLOR );
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < SDL_arraysize(BULLET_VERTICES); ++i)
	{
		const Vector2 &p = BULLET_VERTICES[i];
		glVertex2f(BULLET_HALO_RADIUS*p.x, BULLET_HALO_RADIUS*p.y);
	}
	glEnd();

	// finish the draw list
	glEndList();
}

Bullet::~Bullet(void)
{
}

void Bullet::Simulate(float aStep)
{
	// count down life
	mLife -= aStep;
	if (mLife <= 0)
	{
		delete this;
		return;
	}

	// apply velocity
	pos += vel * aStep;
}

void Bullet::Collide(float aStep, Collidable &aRecipient)
{
	// remove collision
	SetLayer(-1);

	// advance to contact point
	pos += vel * aStep;

	// kill the bullet
	mLife = 0.0f;
	vel.x = 0.0f;
	vel.y = 0.0f;

	// create an explosion
	Explosion *explosion = new Explosion();
	explosion->SetPosition(pos);
}

void Bullet::Render(void)
{
	// push a transform
	glPushMatrix();

	// set offset
	glTranslatef( pos.x, pos.y, 0 );

	// draw bullet
	glCallList(mDraw);

	// if moving...
	if (vel.x != 0 && vel.y != 0)
	{
		// disable 2D texturing
		glDisable( GL_TEXTURE_2D );

		// draw a tail
		float tail = std::min(1.0f - mLife, BULLET_TAIL_LENGTH);
		glBegin( GL_LINES );
		glColor4fv( BULLET_TAIL_COLOR[0] );
		glVertex2f( 0, 0 );
		glColor4fv( BULLET_TAIL_COLOR[1] );
		glVertex2f( vel.x * -tail, vel.y * -tail );
		glEnd();
	}

	// reset the transform
	glPopMatrix();
}
