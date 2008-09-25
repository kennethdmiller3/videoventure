#pragma once

#include "Updatable.h"

class WeaponTemplate
{
public:
	// offset
	Transform2 mOffset;
	Transform2 mScatter;
	Transform2 mInherit;
	Transform2 mVelocity;
	Transform2 mVariance;
	float mRecoil;

	// ordnance
	unsigned int mOrdnance;
	
	// flash (tethered)
	unsigned int mFlash;

	// fire channel
	int mChannel;

	// fire delay
	float mDelay;
	int mPhase;
	int mCycle;
	int mTrack;

	// ammo
	unsigned int mType;
	float mCost;

public:
	WeaponTemplate(void);
	~WeaponTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

class Weapon
	: public Updatable
{
protected:
	// controller
	unsigned int mControlId;

	// fire channel
	int mChannel;

	// fire timer
	int mTrack;
	float mTimer;
	int mPhase;

	// ammo pool
	unsigned int mAmmo;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Weapon(void);
	Weapon(const WeaponTemplate &aTemplate, unsigned int aId);
	virtual ~Weapon(void);

	// set control
	void SetControl(unsigned int aControlId)
	{
		mControlId = aControlId;
	}

	// update
	void Update(float aStep);

protected:
	friend class WeaponTracker;

	// tracking
	void Track(int aAdd)
	{
		mTrack += aAdd;
	}
};

namespace Database
{
	extern Typed<WeaponTemplate> weapontemplate;
	extern Typed<Weapon *> weapon;
}
