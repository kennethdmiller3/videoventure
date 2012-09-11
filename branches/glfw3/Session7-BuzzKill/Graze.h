#pragma once

#include "Updatable.h"
#include "Collidable.h"

class GrazeTemplate
{
public:
	// collision
	CollidableFilter mFilter;

	// ammo type
	unsigned int mType;

	// collector
	Transform2 mOffset;
	float mRadiusInner;
	float mRadiusOuter;
	float mValueInner;
	float mValueOuter;

	// effect
	Transform2 mScatter;
	Transform2 mInherit;
	Transform2 mVelocity;
	Transform2 mVariance;
	unsigned int mSpawn;

	// upgrade
	unsigned int mSwitchOnFull;

public:
	GrazeTemplate(void);
	~GrazeTemplate(void);

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int id);
};


class Graze : public Updatable
{
protected:
	// ammo pool
	unsigned int mAmmo;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Graze(void);
	Graze(const GrazeTemplate &aTemplate, unsigned int aId);
	~Graze(void);

	void Update(float aStep);
};
