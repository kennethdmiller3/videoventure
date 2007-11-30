#pragma once

#include "Entity.h"
#include "Collidable.h"
#include "Renderable.h"

class Target
	: public Entity, public Collidable, public Renderable
{
public:
	Target(void);
	~Target(void);

	// configure
	virtual bool Configure(TiXmlElement *element);

	// collide
	virtual void Collide(float aStep, Collidable &aRecipient);

	// render
	virtual void Render(void);
};
