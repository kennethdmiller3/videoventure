#pragma once

#include "Simulatable.h"

// gunner actor
class Gunner : 
	public Simulatable
{
public:
	// constructor
	Gunner(unsigned int aId = 0);

	// destructor
	~Gunner(void);

	// configure
	virtual bool Configure(const TiXmlElement *element);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<Gunner *> gunner;
}