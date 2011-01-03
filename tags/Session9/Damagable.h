#pragma once

#include "Database.h"

class DamagableTemplate
{
public:
	float mHealth;
	unsigned int mSpawnOnDeath;
	unsigned int mSwitchOnDeath;

	float mPropagateScale;
	float mPropagateDeath;

public:
	DamagableTemplate(void);
	~DamagableTemplate(void);

	bool Configure(const TiXmlElement *element);
};

class Damagable
{
protected:
	unsigned int mId;

	float mHealth;

public:
	typedef fastdelegate::FastDelegate<void (unsigned int, unsigned int, float)> DamageListener;
	typedef fastdelegate::FastDelegate<void (unsigned int, unsigned int)> DeathListener;
	typedef fastdelegate::FastDelegate<void (unsigned int, unsigned int)> KillListener;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Damagable(void);
	Damagable(const DamagableTemplate &aTemplate, unsigned int aId);
	virtual ~Damagable(void);

	void Damage(unsigned int aSourceId, float aDamage);

	void Kill(void);

	float GetHealth(void)
	{
		return mHealth;
	}

	bool IsAlive(void)
	{
		return mHealth > 0.0f;
	}
};

namespace Database
{
	extern Typed<DamagableTemplate> damagabletemplate;
	extern Typed<Damagable *> damagable;
	extern Typed<Typed<Damagable::DamageListener> > damagelistener;
	extern Typed<Typed<Damagable::DeathListener> > deathlistener;
	extern Typed<Typed<Damagable::KillListener> > killlistener;
	extern Typed<int> hitcombo;
}