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
	Explosion(void);
	~Explosion(void);

	// simulate
	void Simulate(float aStep);

	// render
	void Render(void);
};
