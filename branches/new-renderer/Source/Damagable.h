#pragma once

#include "Database.h"
#include "Signal.h"

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

	bool Configure(const tinyxml2::XMLElement *element);
};

class GAME_API Damagable
{
protected:
	unsigned int mId;

	float mHealth;

public:
	typedef Signal<void (unsigned int aId, unsigned int aSourceId, float aDamage)> DamageSignal;
	typedef Signal<void (unsigned int aId, unsigned int aSourceId)> DeathSignal;
	typedef Signal<void (unsigned int aId, unsigned int aKillId)> KillSignal;

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
	extern GAME_API Typed<DamagableTemplate> damagabletemplate;
	extern GAME_API Typed<Damagable *> damagable;
	extern GAME_API Typed<Damagable::DamageSignal > damagesignal;
	extern GAME_API Typed<Damagable::DeathSignal > deathsignal;
	extern GAME_API Typed<Damagable::KillSignal > killsignal;
	extern GAME_API Typed<int> hitcombo;
}
