#pragma once

#include "Database.h"
#include "Updatable.h"

// find the specified resource,
// starting with the specified identifier
// and backtracking through the link chain
extern unsigned int FindResource(unsigned int aId, unsigned int aSubId);

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

class Resource : public Updatable
{
protected:
	unsigned int mSubId;
	float mValue;
	float mTimer;

public:
	typedef fastdelegate::FastDelegate<void (unsigned int, unsigned int, unsigned int, float)> ChangeListener;
	typedef fastdelegate::FastDelegate<void (unsigned int, unsigned int, unsigned int)> EmptyListener;
	typedef fastdelegate::FastDelegate<void (unsigned int, unsigned int, unsigned int)> FullListener;

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

	float GetValue(void)
	{
		return mValue;
	}

	bool IsEmpty(void)
	{
		return mValue <= 0.0f;
	}
};

namespace Database
{
	extern Typed<Typed<ResourceTemplate> > resourcetemplate;
	extern Typed<Typed<Resource *> > resource;
	extern Typed<Typed<Typed<Resource::ChangeListener> > > resourcechangelistener;
	extern Typed<Typed<Typed<Resource::EmptyListener> > > resourceemptylistener;
	extern Typed<Typed<Typed<Resource::FullListener> > > resourcefulllistener;
}
