#pragma once

#include "Simulatable.h"

class SpawnerTemplate
{
public:
	// offset
	Matrix2 mOffset;
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

	bool Configure(TiXmlElement *element);
};

class Spawner
	: public Simulatable
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

	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<SpawnerTemplate> spawnertemplate;
	extern Typed<Spawner *> spawner;
}
