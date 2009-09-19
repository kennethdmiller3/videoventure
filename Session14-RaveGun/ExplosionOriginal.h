#pragma once

#include "Updatable.h"

class ExplosionTemplate
{
public:
	// life span
	float mLifeSpan;

	// collision
	b2Filter mFilter;

	// damage
	float mRadiusInner;
	float mRadiusOuter;
	float mDamageInner;
	float mDamageOuter;

public:
	ExplosionTemplate(void);
	~ExplosionTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element, unsigned int id);
};

class Explosion :
	public Updatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// life
	float mLife;

public:
	Explosion(void);
	Explosion(const ExplosionTemplate &aTemplate, unsigned int aId);
	~Explosion(void);

	// update
	void Update(float aStep);
};

namespace Database
{
	extern Typed<ExplosionTemplate> explosiontemplate;
	extern Typed<Explosion *> explosion;
}