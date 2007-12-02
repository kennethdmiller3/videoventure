#pragma once

#include "Entity.h"
#include "Simulatable.h"
#include "Collidable.h"
#include "Renderable.h"

class Bullet :
	public Entity, public Simulatable, public Collidable, public Renderable
{
public:

	// life
	float mLife;

public:
	Bullet(unsigned int aId = 0, unsigned int aParentId = 0);
	~Bullet(void);

	// simulate
	virtual void Simulate(float aStep);

	// collide
	virtual void Collide(float aStep, Collidable &aRecipient);

	// render
	virtual void Render(void);
};
