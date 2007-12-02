#pragma once
#include "Entity.h"
#include "Simulatable.h"
#include "Renderable.h"

class Explosion :
	public Entity, public Simulatable, public Renderable
{
public:

	// life
	float mLife;

public:
	Explosion(unsigned int aId = 0, unsigned int aParentId = 0);
	~Explosion(void);

	// simulate
	virtual void Simulate(float aStep);

	// render
	virtual void Render(void);
};
