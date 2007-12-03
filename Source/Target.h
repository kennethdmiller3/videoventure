#pragma once

#include "Entity.h"
#include "Collidable.h"
#include "Renderable.h"

class Target
	: public Entity, public Collidable, public Renderable
{
public:
	Target(unsigned int aId = 0, unsigned int aParentId = 0);
	~Target(void);

	// configure
	virtual bool Configure(TiXmlElement *element);

	// collide
	virtual void Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount);

	// render
	virtual void Render(const Matrix2 &transform);
};
