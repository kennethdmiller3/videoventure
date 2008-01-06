#pragma once

#include "Simulatable.h"
#include "Controllable.h"

class WeaponTemplate
{
public:
	// offset
	Matrix2 mOffset;
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
	: public Controllable, public Simulatable
{
protected:
	// control values
	bool mFire;

	// fire delay
	float mDelay;
	int mPhase;

public:
	Weapon(void);
	Weapon(const WeaponTemplate &aTemplate, unsigned int aId);
	virtual ~Weapon(void);

	// configure
	bool Configure(TiXmlElement *element);

	// control
	virtual void Control(float aStep);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<WeaponTemplate> weapontemplate;
	extern Typed<Weapon *> weapon;
}
