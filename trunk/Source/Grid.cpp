#include "StdAfx.h"
#include "Grid.h"

Grid::Grid(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Renderable(Database::renderabletemplate.Get(aParentId))
{
}

Grid::~Grid(void)
{
}

// configure
bool Grid::Configure(TiXmlElement *element)
{
	return Entity::Configure(element) || Renderable::Configure(element);
}

void Grid::Render(const Matrix2 &transform)
{
	// push a transform
	glPushMatrix();

	// call the draw list
	glCallList(mDraw);

	// reset the transform
	glPopMatrix();
}