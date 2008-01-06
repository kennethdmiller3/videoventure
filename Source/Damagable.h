#pragma once

#include "Database.h"

class DamagableTemplate
{
public:
	float mHealth;
	unsigned int mSpawnOnDeath;

public:
	DamagableTemplate(void);
	~DamagableTemplate(void);

	bool Configure(TiXmlElement *element);
};

class Damagable
{
protected:
	unsigned int id;

	float mHealth;

public:
	typedef fastdelegate::FastDelegate<void (unsigned int, float)> Listener;

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
	extern Typed<Typed<Damagable::Listener> > damagablelistener;
}
