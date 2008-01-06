#pragma once

#include "Entity.h"
#include "Controllable.h"
#include "Simulatable.h"
#include "Renderable.h"
#include "Input.h"
#include "Player.h"

// gunner actor
class Gunner : 
	public Simulatable
{
protected:
	// owner
	unsigned int owner;

public:
	// constructor
	Gunner(unsigned int aId = 0, unsigned int aParentId = 0);

	// destructor
	~Gunner(void);

	// configure
	virtual bool Configure(TiXmlElement *element);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<Gunner *> gunner;
}