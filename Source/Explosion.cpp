#include "StdAfx.h"
#include "Explosion.h"

// explosion attributes
const float EXPLOSION_RADIUS = 6;
const float EXPLOSION_LIFE = 0.25f;

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


// explosion pool
boost::object_pool<Explosion> Explosion::pool;


Explosion::Explosion(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Simulatable()
, Renderable(Database::renderabletemplate.Get(aParentId))
, mLife(EXPLOSION_LIFE)
{
	// get a circle drawlist
	mDraw = Database::drawlist.Get(0x8cdedbba /* "circle16" */);

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
		pool.destroy(this);
		return;
	}
}

void Explosion::Render(void)
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
