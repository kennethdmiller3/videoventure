#pragma once

#include "Controller.h"
#include "Behavior/Brain.h"

// forward declaration
class Entity;
class Behavior;

// aimer template
class AimerTemplate
{
public:
	float mSide;
	float mFront;
	float mTurn;
	Transform2 mDrift;
	std::vector<unsigned int> mBehaviors;

public:
	AimerTemplate(void);
	~AimerTemplate(void);

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

// aimer controller
class Aimer :
	public Controller, public Brain
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Aimer(const AimerTemplate &aTemplate, unsigned int aId = 0);
	virtual ~Aimer(void);

	// control
	void Control(float aStep);
};

namespace Database
{
	extern Typed<AimerTemplate> aimertemplate;
	extern Typed<Aimer *> aimer;
}