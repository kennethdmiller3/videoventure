#pragma once

#include "Simulatable.h"

class WeaponTemplate
{
public:
	// offset
	Matrix2 mOffset;
	Vector2 mInherit;
	Vector2 mVelocity;

	// ordnance
	unsigned int mOrdnance;

	// fire delay
	float mDelay;
	int mPhase;
	int mCycle;

public:
	WeaponTemplate(void);
	~WeaponTemplate(void);

	// configure
	bool Configure(TiXmlElement *element);
};

class Weapon
	: public Simulatable
{
protected:
	// fire timer
	float mTimer;
	int mPhase;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Weapon(void);
	Weapon(const WeaponTemplate &aTemplate, unsigned int aId);
	virtual ~Weapon(void);

	// configure
	bool Configure(TiXmlElement *element);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<WeaponTemplate> weapontemplate;
	extern Typed<Weapon *> weapon;
}
