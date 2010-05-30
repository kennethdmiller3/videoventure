#pragma once

#include "Database.h"
#include "Updatable.h"
#include "Signal.h"

// find the specified resource,
// starting with the specified identifier
// and backtracking through the link chain
extern GAME_API unsigned int FindResourceContainer(unsigned int aId, unsigned int aSubId);

class ResourceTemplate
{
public:
	// resource name hash
	unsigned int mSubId;

	// initial value
	float mInitial;

	// maximum value
	float mMaximum;

	// recovery/decay
	float mDelay;
	float mCycle;
	float mAdd;

public:
	ResourceTemplate(void);
	~ResourceTemplate(void);

	bool Configure(const TiXmlElement *element, unsigned int aId, unsigned int aSubId);
};

class GAME_API Resource : public Updatable
{
protected:
	unsigned int mSubId;
	float mValue;
	float mTimer;

public:
	typedef Signal<void (unsigned int, unsigned int, unsigned int, float)> ChangeSignal;
	typedef Signal<void (unsigned int, unsigned int, unsigned int)> EmptySignal;
	typedef Signal<void (unsigned int, unsigned int, unsigned int)> FullSignal;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Resource(void);
	Resource(const ResourceTemplate &aTemplate, unsigned int aId);
	virtual ~Resource(void);

	void Set(unsigned int aSourceId, float aValue);
	void Add(unsigned int aSourceId, float aAdd);

	void Update(float aStep);

	float GetValue(void) const
	{
		return mValue;
	}

	bool IsEmpty(void) const
	{
		return mValue <= 0.0f;
	}
};

namespace Database
{
	extern GAME_API Typed<Typed<ResourceTemplate> > resourcetemplate;
	extern GAME_API Typed<Typed<Resource *> > resource;
	extern GAME_API Typed<Typed<Resource::ChangeSignal> > resourcechange;
	extern GAME_API Typed<Typed<Resource::EmptySignal> > resourceempty;
	extern GAME_API Typed<Typed<Resource::FullSignal> > resourcefull;
}
