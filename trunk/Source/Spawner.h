#pragma once

#include "Updatable.h"

class SpawnerTemplate
{
public:
	// offset
	Matrix2 mOffset;
	Vector2 mInherit;
	Vector2 mVelocity;

	// item to spawn
	unsigned int mSpawn;

	// spawn cycle
	float mStart;
	float mCycle;
	bool mTrack;

public:
	SpawnerTemplate(void);
	~SpawnerTemplate(void);

	bool Configure(const TiXmlElement *element);
};

class Spawner
	: public Updatable
{
protected:
	unsigned int mSpawn;
	float mTimer;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Spawner(void);
	Spawner(const SpawnerTemplate &aTemplate, unsigned int aId);
	virtual ~Spawner(void);

	// update
	void Update(float aStep);
};

namespace Database
{
	extern Typed<SpawnerTemplate> spawnertemplate;
	extern Typed<Spawner *> spawner;
}
