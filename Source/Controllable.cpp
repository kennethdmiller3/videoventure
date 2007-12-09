#include "StdAfx.h"
#include "Controllable.h"

Controllable::List Controllable::sAll;

Controllable::Controllable(unsigned int aId)
: id(aId), entry(sAll.end())
{
	if (id > 0)
		entry = sAll.insert(sAll.end(), this);
}

Controllable::~Controllable(void)
{
	if (entry != sAll.end())
		sAll.erase(entry);
}

void Controllable::ControlAll(float aStep)
{
	// control all controllables
	List::iterator itor = sAll.begin();
	while (itor != sAll.end())
	{
		// get the next iterator
		// (in case the entry gets deleted)
		List::iterator next(itor);
		++next;

		// control
		(*itor)->Control(aStep);

		// go to the next iterator
		itor = next;
	}
}
