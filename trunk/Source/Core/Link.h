#pragma once

#include "Updatable.h"

class GAME_API LinkTemplate
{
public:
	// offset
	Transform2 mOffset;

	// sub-id
	unsigned int mSub;

	// secondary
	unsigned int mSecondary;

	// type-id
	unsigned int mType;

	// update flags
	bool mUpdateAngle;
	bool mUpdatePosition;
	bool mDeleteSecondary;

public:
	LinkTemplate(void);
	~LinkTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId, unsigned int aSubId);
};

class GAME_API Link :
	public Updatable
{
protected:
	unsigned int mSub;

	// secondary
	unsigned int mSecondary;

	// offset
	Transform2 mOffset;

	// update flags
	bool mUpdateAngle;
	bool mUpdatePosition;
	bool mDeleteSecondary;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Link(void);
	Link(const LinkTemplate &aTemplate, unsigned int aId);
	virtual ~Link(void);

	unsigned int GetSecondary(void) const
	{
		return mSecondary;
	}

	// update
	void Update(float aStep);
};

namespace Database
{
	extern GAME_API Typed<Typed<LinkTemplate> > linktemplate;
	extern GAME_API Typed<Typed<Link *> > link;
	extern GAME_API Typed<unsigned int> backlink;
}
