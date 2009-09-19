#pragma once

#include "Updatable.h"

class LinkTemplate
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
	bool mUpdateTeam;
	bool mDeleteSecondary;

public:
	LinkTemplate(void);
	~LinkTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId, unsigned int aSubId);
};

class Link :
	public Updatable
{
protected:
	unsigned int mSub;
	unsigned int mSecondary;

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
	extern Typed<Typed<LinkTemplate> > linktemplate;
	extern Typed<Typed<Link *> > link;
	extern Typed<unsigned int> backlink;
	extern Typed<bool> below;
}
