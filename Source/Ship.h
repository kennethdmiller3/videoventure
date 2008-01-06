#pragma once

#include "Simulatable.h"

// ship template
class ShipTemplate
{
public:
	// physics parameters
	float mMaxVeloc;
	float mMaxAccel;
	float mFriction;
	float mMaxOmega;

public:
	ShipTemplate(void);
	~ShipTemplate(void);

	// configure
	bool Configure(TiXmlElement *element);
};

// ship actor
class Ship : 
	public Simulatable
{
public:
	// constructor
	Ship(void);
	Ship(const ShipTemplate &aTemplate, unsigned int aId = 0);

	// destructor
	~Ship(void);

	// configure
	virtual bool Configure(TiXmlElement *element);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<ShipTemplate> shiptemplate;
	extern Typed<Ship *> ship;
}