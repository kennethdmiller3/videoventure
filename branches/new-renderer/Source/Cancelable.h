#pragma once

class CancelableTemplate
{
public:
	unsigned int mSpawn;
	unsigned int mSwitch;
	bool mTethered;
	float mBacklash;

public:
	CancelableTemplate(void);
	~CancelableTemplate(void);

	// configure
	bool Configure(const tinyxml2::XMLElement *element);
};

class Cancelable
{
protected:
	unsigned int mId;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// constructor
	Cancelable(void);
	Cancelable(const CancelableTemplate &aTemplate, unsigned int aId);

	// destructor
	virtual ~Cancelable(void);

	// cancel
	void Cancel(unsigned int aId, unsigned int aSourceId);

protected:
	// creator death
	void CreatorDeath(unsigned int aId, unsigned int aSourceId);
};

namespace Database
{
	extern Typed<CancelableTemplate> cancelabletemplate;
	extern Typed<Cancelable *> cancelable;
}
