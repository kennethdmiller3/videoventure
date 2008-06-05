#pragma once

#include "Simulatable.h"

// ship template
class ShipTemplate
{
public:
	// physics parameters
	float mReverseVeloc;
	float mNeutralVeloc;
	float mForwardVeloc;
	float mStrafeVeloc;
	float mMinAccel;
	float mMaxAccel;
	float mMaxOmega;

public:
	ShipTemplate(void);
	~ShipTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

// ship actor
class Ship : 
	public Simulatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// constructor
	Ship(void);
	Ship(const ShipTemplate &aTemplate, unsigned int aId = 0);

	// destructor
	~Ship(void);

	// configure
	virtual bool Configure(const TiXmlElement *element);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<ShipTemplate> shiptemplate;
	extern Typed<Ship *> ship;
}