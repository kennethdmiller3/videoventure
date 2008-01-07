#include "StdAfx.h"
#include "Updatable.h"

Updatable::List Updatable::sAll;

Updatable::Updatable(unsigned int aId)
: id(aId), entry(sAll.end())
{
	if (id > 0)
		entry = sAll.insert(sAll.end(), Entry(this, &Updatable::Update));
}

Updatable::~Updatable(void)
{
	if (entry != sAll.end())
		sAll.erase(entry);
}

void Updatable::UpdateAll(float aStep)
{
	// update all updatables
	List::iterator itor = sAll.begin();
	while (itor != sAll.end())
	{
		// get the next iterator
		// (in case the entry gets deleted)
		List::iterator next(itor);
		++next;

		// update
		(*itor)(aStep);

		// go to the next iterator
		itor = next;
	}
}
