#pragma once

#include "Updatable.h"
#include "Collidable.h"

class BeamTemplate
{
public:
	// life span
	float mLifeSpan;

	// damage
	float mDamage;
	float mRange;

	// collision
	CollidableFilter mFilter;

	// spawn
	unsigned int mSpawnOnImpact;

public:
	BeamTemplate(void);
	~BeamTemplate(void);

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int id);
};

class Beam :
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
	Beam(void);
	Beam(const BeamTemplate &aTemplate, unsigned int aId);
	~Beam(void);

	// update
	void Update(float aStep);
};

namespace Database
{
	extern Typed<BeamTemplate> beamtemplate;
	extern Typed<Beam *> beam;
}