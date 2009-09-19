#pragma once

#include "Updatable.h"

class WeaponTemplate
{
public:
	// transforms
	struct Transforms
	{
		Transform2 mOffset;
		Transform2 mScatter;
		Transform2 mInherit;
		Transform2 mVelocity;
		Transform2 mVariance;
		Vector2 mAim;
	};

	// base transform
	Transforms mBase;

	// delta transform per burst
	Transforms mBurst;

	// delta transform per salvo
	Transforms mSalvo;

	// recoil impulse
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

	// burst
	int mBurstLength;
	float mBurstDelay;

	// salvo
	int mSalvoShots;

	// ammo
	unsigned int mType;
	float mCost;

public:
	WeaponTemplate(void);
	~WeaponTemplate(void);

	// configure
	bool ProcessTransformsItem(const TiXmlElement *element, unsigned int aId, Transforms &aTransforms);
	bool ConfigureTransforms(const TiXmlElement *element, unsigned int aId, Transforms &aTransforms);
	bool Configure(const TiXmlElement *element, unsigned int aId);
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
	int mBurst;
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
