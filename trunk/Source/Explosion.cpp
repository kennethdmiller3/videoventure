#include "StdAfx.h"
#include "Explosion.h"

// explosion attributes
const float EXPLOSION_RADIUS = 6;
const float EXPLOSION_LIFE = 0.25f;

// explosion geometry (unscaled)
const float EXPLOSION_ANGLE = (float)M_PI/8;
const Vector2 EXPLOSION_VERTICES[] =
{
	Vector2(cosf( 0*EXPLOSION_ANGLE), sinf( 0*EXPLOSION_ANGLE)),
	Vector2(cosf( 1*EXPLOSION_ANGLE), sinf( 1*EXPLOSION_ANGLE)),
	Vector2(cosf(15*EXPLOSION_ANGLE), sinf(15*EXPLOSION_ANGLE)),
	Vector2(cosf( 2*EXPLOSION_ANGLE), sinf( 2*EXPLOSION_ANGLE)),
	Vector2(cosf(14*EXPLOSION_ANGLE), sinf(14*EXPLOSION_ANGLE)),
	Vector2(cosf( 3*EXPLOSION_ANGLE), sinf( 3*EXPLOSION_ANGLE)),
	Vector2(cosf(13*EXPLOSION_ANGLE), sinf(13*EXPLOSION_ANGLE)),
	Vector2(cosf( 4*EXPLOSION_ANGLE), sinf( 4*EXPLOSION_ANGLE)),
	Vector2(cosf(12*EXPLOSION_ANGLE), sinf(12*EXPLOSION_ANGLE)),
	Vector2(cosf( 5*EXPLOSION_ANGLE), sinf( 5*EXPLOSION_ANGLE)),
	Vector2(cosf(11*EXPLOSION_ANGLE), sinf(11*EXPLOSION_ANGLE)),
	Vector2(cosf( 6*EXPLOSION_ANGLE), sinf( 6*EXPLOSION_ANGLE)),
	Vector2(cosf(10*EXPLOSION_ANGLE), sinf(10*EXPLOSION_ANGLE)),
	Vector2(cosf( 7*EXPLOSION_ANGLE), sinf( 7*EXPLOSION_ANGLE)),
	Vector2(cosf( 9*EXPLOSION_ANGLE), sinf( 9*EXPLOSION_ANGLE)),
	Vector2(cosf( 8*EXPLOSION_ANGLE), sinf( 8*EXPLOSION_ANGLE)),
};

// explosion core geometry
const float EXPLOSION_CORE_RADIUS[2] = { 6, 3 };
const float EXPLOSION_CORE_COLOR[2][4] =
{
	{ 0.8f, 0.9f, 1.0f, 1.0f },
	{ 0.0f, 0.5f, 1.0f, 0.0f }
};

// explosion halo geometry
const float EXPLOSION_HALO_RADIUS[2] = { 8, 16 };
const float EXPLOSION_HALO_COLOR[2][4] =
{
	{ 0.4f, 0.7f, 1.0f, 0.5f },
	{ 0.0f, 0.0f, 1.0f, 0.0f }
};



Explosion::Explosion(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Simulatable()
, Renderable(Database::renderabletemplate.Get(aParentId))
, mLife(EXPLOSION_LIFE)
{
	// create a new draw list
	mDraw = glGenLists(1);
	glNewList(mDraw, GL_COMPILE);

	// generate circle
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < SDL_arraysize(EXPLOSION_VERTICES); ++i)
	{
		const Vector2 &p = EXPLOSION_VERTICES[i];
		glVertex2f(p.x, p.y);
	}
	glEnd();

	// finish the draw list
	glEndList();

	// set as visible
	Renderable::Show();
}

Explosion::~Explosion(void)
{
}

void Explosion::Simulate(float aStep)
{
	// count down life
	mLife -= aStep;
	if (mLife <= 0)
	{
		delete this;
		return;
	}
}

void Explosion::Render(void)
{
	// push a transform
	glPushMatrix();

	// set offset
	glTranslatef( transform.p.x, transform.p.y, 0 );

	// interpolation factor
	float f = 1.0f - mLife / EXPLOSION_LIFE;

	// draw explosion core
	glColor4f(
		EXPLOSION_CORE_COLOR[0][0] + (EXPLOSION_CORE_COLOR[1][0] - EXPLOSION_CORE_COLOR[0][0]) * f,
		EXPLOSION_CORE_COLOR[0][1] + (EXPLOSION_CORE_COLOR[1][1] - EXPLOSION_CORE_COLOR[0][1]) * f,
		EXPLOSION_CORE_COLOR[0][2] + (EXPLOSION_CORE_COLOR[1][2] - EXPLOSION_CORE_COLOR[0][2]) * f,
		EXPLOSION_CORE_COLOR[0][3] + (EXPLOSION_CORE_COLOR[1][3] - EXPLOSION_CORE_COLOR[0][3]) * f
	);
	float r_core = EXPLOSION_CORE_RADIUS[0] + (EXPLOSION_CORE_RADIUS[1] - EXPLOSION_CORE_RADIUS[0]) * f;
	glPushMatrix();
	glScalef(r_core, r_core, r_core);
	glCallList(mDraw);
	glPopMatrix();

	// draw explosion halo
	glColor4f(
		EXPLOSION_HALO_COLOR[0][0] + (EXPLOSION_HALO_COLOR[1][0] - EXPLOSION_HALO_COLOR[0][0]) * f,
		EXPLOSION_HALO_COLOR[0][1] + (EXPLOSION_HALO_COLOR[1][1] - EXPLOSION_HALO_COLOR[0][1]) * f,
		EXPLOSION_HALO_COLOR[0][2] + (EXPLOSION_HALO_COLOR[1][2] - EXPLOSION_HALO_COLOR[0][2]) * f,
		EXPLOSION_HALO_COLOR[0][3] + (EXPLOSION_HALO_COLOR[1][3] - EXPLOSION_HALO_COLOR[0][3]) * f
	);
	float r_halo = EXPLOSION_HALO_RADIUS[0] + (EXPLOSION_HALO_RADIUS[1] - EXPLOSION_HALO_RADIUS[0]) * f;
	glPushMatrix();
	glScalef(r_halo, r_halo, r_halo);
	glCallList(mDraw);
	glPopMatrix();

	// reset the transform
	glPopMatrix();
}
