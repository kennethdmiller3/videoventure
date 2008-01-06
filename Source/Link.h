#pragma once
#include "simulatable.h"

class LinkTemplate
{
public:
	// offset
	Matrix2 mOffset;

	// sub-id
	unsigned int mSub;

	// secondary
	unsigned int mSecondary;

public:
	LinkTemplate(void);
	~LinkTemplate(void);

	// configure
	bool Configure(TiXmlElement *element);
};

class Link :
	public Simulatable
{
protected:
	unsigned int mSub;
	unsigned int mSecondary;

public:
	Link(void);
	Link(const LinkTemplate &aTemplate, unsigned int aId);
	virtual ~Link(void);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<Typed<LinkTemplate> > linktemplate;
	extern Typed<Typed<Link *> > link;
}
