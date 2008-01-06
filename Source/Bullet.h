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

public:
	BulletTemplate(void);
	~BulletTemplate(void);

	// configure
	bool Configure(TiXmlElement *element);
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
	virtual void Simulate(float aStep);

	// collide
	void Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount);
};

namespace Database
{
	extern Typed<BulletTemplate> bullettemplate;
	extern Typed<Bullet *> bullet;
}
