#include "StdAfx.h"
#include "Grid.h"

Grid::Grid(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Renderable(Database::renderabletemplate.Get(aParentId))
{
	// create a new draw list
	mDraw = glGenLists(1);
	glNewList(mDraw, GL_COMPILE);

	glBegin( GL_LINES );

	// draw perimeter
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glVertex3i(ARENA_X_MIN, ARENA_Y_MIN, 0);
	glVertex3i(ARENA_X_MIN, ARENA_Y_MAX, 0);
	glVertex3i(ARENA_X_MAX, ARENA_Y_MIN, 0);
	glVertex3i(ARENA_X_MAX, ARENA_Y_MAX, 0);
	glVertex3i(ARENA_X_MIN, ARENA_Y_MIN, 0);
	glVertex3i(ARENA_X_MAX, ARENA_Y_MIN, 0);
	glVertex3i(ARENA_X_MIN, ARENA_Y_MAX, 0);
	glVertex3i(ARENA_X_MAX, ARENA_Y_MAX, 0);

	// draw grid
	for (int x = ARENA_X_MIN + 64, i = 1; x <= ARENA_X_MAX - 64; x += 64, ++i)
	{
		glColor4f(1.0f, 1.0f, 1.0f, (i & 3) ? 0.0625f : 0.25f);
		glVertex3i(x, ARENA_Y_MIN, 0);
		glVertex3i(x, ARENA_Y_MAX, 0);
	}
	for (int y = ARENA_Y_MIN + 64, i = 1; y <= ARENA_Y_MAX - 64; y += 64, ++i)
	{
		glColor4f(1.0f, 1.0f, 1.0f, (i & 3) ? 0.0625f : 0.25f);
		glVertex3i(ARENA_X_MIN, y, 0);
		glVertex3i(ARENA_X_MAX, y, 0);
	}

	glEnd();

	// finish the draw list
	glEndList();

	// set as visible
	Renderable::Show();
}

Grid::~Grid(void)
{
}

void Grid::Render(void)
{
	// call the draw list
	glCallList(mDraw);
}