#include "StdAfx.h"
#include "Bullet.h"
#include "Explosion.h"

// bullet attributes
const float BULLET_RADIUS = 3;
const float BULLET_LIFE = 1.0f;

// bullet tail geometry
const float BULLET_TAIL_LENGTH = 0.0625f;
const float BULLET_TAIL_COLOR[2][4] =
{
	{ 1.0f, 1.0f, 1.0f, 0.5f },
	{ 0.0f, 0.0f, 1.0f, 0.0f }
};

Bullet::Bullet(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Simulatable()
, Collidable(Database::collidabletemplate.Get(aParentId))
, Renderable(Database::renderabletemplate.Get(aParentId))
, mLife(BULLET_LIFE)
{
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
	transform.p += vel * aStep;
}

void Bullet::Collide(float aStep, Collidable &aRecipient)
{
	// remove collision
	SetLayer(-1);

	// kill the bullet
	mLife = 0.0f;
	vel.x = 0.0f;
	vel.y = 0.0f;

	// create an explosion
	Explosion *explosion = new Explosion(0, 0x70f5d327 /* "playerbulletexplosion" */);
	explosion->SetPosition(transform.p);
}

void Bullet::Render(void)
{
	// push a transform
	glPushMatrix();

	// set offset
	glTranslatef( transform.p.x, transform.p.y, 0 );

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
