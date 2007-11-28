#include "StdAfx.h"
#include "Simulatable.h"

Simulatable::List Simulatable::sAll;

Simulatable::Simulatable(void)
{
	entry = sAll.insert(sAll.end(), this);
}

Simulatable::~Simulatable(void)
{
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
		(*itor)->Simulate(aStep);

		// go to the next iterator
		itor = next;
	}
}
