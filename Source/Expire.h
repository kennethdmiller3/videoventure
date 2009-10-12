#pragma once

#include "Updatable.h"

class ExpireTemplate
{
public:
	float mTime;
	unsigned int mSpawn;
	unsigned int mSwitch;
	bool mReticule;

public:
	ExpireTemplate(void);
	~ExpireTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

class GAME_API Expire : public Updatable
{
public:
	unsigned int mTurn;
	float mFraction;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// constructor
	Expire(void);
	Expire(const ExpireTemplate &aTemplate, unsigned int aId);

	// destructor
	virtual ~Expire(void);

	// update
	void Update(float aStep);
};

namespace Database
{
	extern GAME_API Typed<ExpireTemplate> expiretemplate;
	extern GAME_API Typed<Expire *> expire;
}
