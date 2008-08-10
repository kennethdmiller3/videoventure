#pragma once

#include "Entity.h"
#include "Updatable.h"
#include "Renderable.h"

class BeamTemplate
{
public:
	// life span
	float mLifeSpan;

	// damage
	float mDamage;
	float mDamageRate;
	float mRange;

	// collision
	unsigned short mCategoryBits;
	unsigned short mMaskBits;

	// spawn
	unsigned int mSpawnOnExpire;
	unsigned int mSpawnOnImpact;

public:
	BeamTemplate(void);
	~BeamTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element, unsigned int id);
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
	virtual void Update(float aStep);
};

namespace Database
{
	extern Typed<BeamTemplate> beamtemplate;
	extern Typed<Beam *> beam;
}