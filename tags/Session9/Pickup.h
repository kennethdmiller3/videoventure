#pragma once

#include "Simulatable.h"

class PickupTemplate
{
public:
	// spawn on collect
	unsigned int mSpawnOnCollect;
	unsigned int mSwitchOnCollect;

public:
	PickupTemplate(void);
	~PickupTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class Pickup
{
protected:
	unsigned int mId;
	bool mDestroy;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

public:
	Pickup(void);
	Pickup(const PickupTemplate &aTemplate, unsigned int aId);
	~Pickup(void);

	// collide
	void Collide(unsigned int aId, unsigned int aHitId, float aTime, const b2ContactPoint &aPoint);

	// kill
	void Kill(float aFraction = 0.0f);
};

namespace Database
{
	extern Typed<PickupTemplate> pickuptemplate;
	extern Typed<Pickup *> pickup;
}
