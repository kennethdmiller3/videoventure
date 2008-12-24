#pragma once

#include "Simulatable.h"

class BulletTemplate
{
public:
	// damage value
	float mDamage;

	// ricochet?
	bool mRicochet;

	// spawn on death
	unsigned int mSpawnOnImpact;
	unsigned int mSpawnOnDeath;
	unsigned int mSwitchOnDeath;

public:
	BulletTemplate(void);
	~BulletTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

class Bullet
{
protected:
	unsigned int mId;
	bool mDestroy;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

public:
	Bullet(void);
	Bullet(const BulletTemplate &aTemplate, unsigned int aId);
	~Bullet(void);

	// collide
	void Collide(unsigned int aId, unsigned int aHitId, float aTime, const b2ContactPoint &aPoint);

	// kill
	void Kill(float aFraction = 0.0f);
};

namespace Database
{
	extern Typed<BulletTemplate> bullettemplate;
	extern Typed<Bullet *> bullet;
}
