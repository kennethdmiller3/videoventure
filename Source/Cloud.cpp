#include "StdAfx.h"
#include "Cloud.h"

float CLOUD_SIZE_MEAN = 256;
float CLOUD_SIZE_VARIANCE = 192;

inline float rand_float()
{
	return (float)rand() * (1.0f / (float)RAND_MAX);
}

Cloud::Cloud(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Renderable(Database::renderabletemplate.Get(aParentId))
{
}

void Cloud::Init(int aCount)
{
	// remove existing draw list
	if (mDraw)
	{
		glDeleteLists(mDraw, 1);
	}

	// create a new draw list
	mDraw = glGenLists(1);
	glNewList(mDraw, GL_COMPILE);

	// begin primitive
	glBegin( GL_QUADS );

	// for each cloud...
	for (int i = 0; i < aCount; i++)
	{
		// randomize position
		float x = ARENA_X_MIN + rand_float() * (ARENA_X_MAX - ARENA_X_MIN);
		float y = ARENA_Y_MIN + rand_float() * (ARENA_Y_MAX - ARENA_Y_MIN);
#ifdef DRAW_FRONT_TO_BACK
		float z = 1.0f + i * (3.0f / aCount);
#else
		float z = 1.0f + (aCount - 1 - i) * (3.0f / aCount);
#endif

		// randomize size
		float w = (rand_float() - rand_float()) * CLOUD_SIZE_VARIANCE + CLOUD_SIZE_MEAN;
		float h = (rand_float() - rand_float()) * CLOUD_SIZE_VARIANCE + CLOUD_SIZE_MEAN;

		// randomize color
		glColor4f(
			rand_float() * 0.25f + 0.25f,
			rand_float() * 0.25f + 0.25f,
			rand_float() * 0.25f + 0.25f,
			0.75f
			);

		// submit vertices
		glVertex3f( x - w, y - h, z );
		glVertex3f( x + w, y - h, z );
		glVertex3f( x + w, y + h, z );
		glVertex3f( x - w, y + h, z );
	}

	// end primitive
	glEnd();

	// finish the draw list
	glEndList();
}

Cloud::~Cloud(void)
{
}

void Cloud::Render(const Matrix2 &transform)
{
	// call draw list
	glCallList( mDraw );
}
