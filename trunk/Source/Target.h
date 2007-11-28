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

	// collide
	void Collide(float aStep, Collidable &aRecipient);

	// render
	void Render(void);
};
