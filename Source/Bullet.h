#pragma once

#include "Simulatable.h"
#include "Collidable.h"

class BulletTemplate
{
public:
	// life span
	float mLife;

	// damage value
	float mDamage;

	// ricochet?
	bool mRicochet;

	// spawn on death
	unsigned int mSpawnOnExpire;
	unsigned int mSpawnOnDeath;
	unsigned int mSpawnOnImpact;

public:
	BulletTemplate(void);
	~BulletTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

class Bullet
	: public Simulatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// life timer
	float mLife;

public:
	Bullet(void);
	Bullet(const BulletTemplate &aTemplate, unsigned int aId);
	~Bullet(void);

	// simulate
	void Simulate(float aStep);

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
