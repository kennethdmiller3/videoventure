#pragma once

#include "Updatable.h"

class ChargeStateTemplate
{
public:
	// next charge state
	unsigned int mNext;

	// state time
	float mTime;

	// ammo cost
	float mCost;

	// action
	std::vector<unsigned int> mAction;

public:
	ChargeStateTemplate();
	~ChargeStateTemplate();

	// configure
	bool ConfigureAction(const TiXmlElement *element, unsigned int aId);
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class ChargeWeaponTemplate
{
public:
	// fire channel
	int mChannel;

	// ammo type
	unsigned int mType;

	// starting state
	unsigned int mStart;

public:
	ChargeWeaponTemplate(void);
	~ChargeWeaponTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class ChargeWeapon
	: public Updatable
{
protected:
	// controller
	unsigned int mControlId;

	// fire channel
	int mChannel;

	// fire state
	unsigned int mState;

	// fire timer
	size_t mIndex;
	float mTimer;
	float mLocal;

	// ammo pool
	unsigned int mAmmo;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	ChargeWeapon(void);
	ChargeWeapon(const ChargeWeaponTemplate &aTemplate, unsigned int aId);
	virtual ~ChargeWeapon(void);

	// set control
	void SetControl(unsigned int aControlId)
	{
		mControlId = aControlId;
	}

	// update
	void UpdateNone(float aStep);
	void UpdateReady(float aStep);
	void UpdateCharge(float aStep);
	void UpdateAction(float aStep);
	void UpdateDelay(float aStep);
};

namespace Database
{
	extern Typed<ChargeWeaponTemplate> chargeweapontemplate;
	extern Typed<ChargeWeapon *> chargeweapon;
}
