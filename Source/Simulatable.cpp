#include "StdAfx.h"
#include "Simulatable.h"

Simulatable::List Simulatable::sAll;

Simulatable::Simulatable(unsigned int aId)
: id(aId), entry(sAll.end())
{
	if (id > 0)
		entry = sAll.insert(sAll.end(), Entry(this, &Simulatable::Simulate));
}

Simulatable::~Simulatable(void)
{
	if (entry != sAll.end())
		sAll.erase(entry);
}

void Simulatable::SimulateAll(float aStep)
{
	// simulate all simulatables
	List::iterator itor = sAll.begin();
	while (itor != sAll.end())
	{
		// get the next iterator
		// (in case the entry gets deleted)
		List::iterator next(itor);
		++next;

		// simulate
		(*itor)(aStep);

		// go to the next iterator
		itor = next;
	}
}
