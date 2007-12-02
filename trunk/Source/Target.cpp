#include "stdafx.h"
#include "Target.h"

// Target constructor
Target::Target(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Collidable(Database::collidabletemplate.Get(aParentId))
, Renderable(Database::renderabletemplate.Get(aParentId))
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

void Target::Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount)
{
}

void Target::Render(void)
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

	// call the draw list
	glCallList( mDraw );

	// reset the transform
	glPopMatrix();
}
