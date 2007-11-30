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

// configure
bool Target::Configure(TiXmlElement *element)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x32608848 /* "target" */:
		{
		}
		return true;

	default:
		return Entity::Configure(element) || Collidable::Configure(element) || Renderable::Configure(element);
	}
}

void Target::Collide(float aStep, Collidable &aRecipient)
{
}

void Target::Render(void)
{
	// push a transform
	glPushMatrix();

	// set offset
	glTranslatef( transform.p.x, transform.p.y, 0 );

	// call the draw list
	glCallList( mDraw );

	// reset the transform
	glPopMatrix();
}
