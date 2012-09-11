#pragma once

#include "Updatable.h"
#include "Collidable.h"

class ExplosionTemplate
{
public:
	// life span
	float mLifeSpan;

	// collision
	CollidableFilter mFilter;

	// properties
	std::vector<unsigned int> mRadius;
	std::vector<unsigned int> mDamage;

public:
	ExplosionTemplate(void);
	~ExplosionTemplate(void);

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int id);
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