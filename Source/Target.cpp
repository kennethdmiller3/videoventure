#include "stdafx.h"
#include "Target.h"

// Target constructor
Target::Target(void)
	: Entity(), Collidable(), Renderable()
{
}

// Target destructor
Target::~Target()
{
}

void Target::Collide(float aStep, Collidable &aRecipient)
{
}

void Target::Render(void)
{
	// push a transform
	glPushMatrix();

	// set offset
	glTranslatef( pos.x, pos.y, 0 );

	// call the draw list
	glCallList( mDraw );

	// reset the transform
	glPopMatrix();
}
