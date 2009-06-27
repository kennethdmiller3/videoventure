#pragma once

#include "Updatable.h"

class WeaponTemplate
{
public:
	// offset
	Vector2 mAim;
	float mRecoil;

	// fire channel
	enum TriggerType
	{
		TRIGGER_HOLD,
		TRIGGER_PRESS,
		TRIGGER_RELEASE
	};
	TriggerType mTrigger;
	int mChannel;

	// fire delay
	float mDelay;
	int mPhase;
	int mCycle;
	int mTrack;

	// ammo
	unsigned int mType;
	float mCost;

	// action
	std::vector<unsigned int> mAction;

public:
	WeaponTemplate(void);
	~WeaponTemplate(void);

	// configure
	bool ConfigureAction(const TiXmlElement *element, unsigned int aId);
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
	float mPrevFire;

	// fire timer
	int mTrack;
	size_t mIndex;
	float mTimer;
	float mLocal;
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

	// set previous fire value (HACK)
	void SetPrevFire(float aFire)
	{
		mPrevFire = aFire;
	}

	// update
	void UpdateNone(float aStep);
	void UpdateReady(float aStep);
	void UpdateAction(float aStep);
	void UpdateDelay(float aStep);

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
