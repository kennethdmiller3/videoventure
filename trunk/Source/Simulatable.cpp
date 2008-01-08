#include "StdAfx.h"
#include "Simulatable.h"

Simulatable::List Simulatable::sAll;
Simulatable::Remove Simulatable::sRemove;

Simulatable::Simulatable(unsigned int aId)
: id(aId), entry(sAll.end())
{
	if (id > 0)
		Activate();
}

Simulatable::~Simulatable(void)
{
	Deactivate();
}

void Simulatable::Activate(void)
{
	if (entry == sAll.end())
	{
		entry = sAll.insert(sAll.end(), Entry(this, &Simulatable::Simulate));
	}
}

void Simulatable::Deactivate(void)
{
	if (entry != sAll.end())
	{
		sRemove.push_back(entry);
		entry->clear();
		entry = sAll.end();
	}
}

void Simulatable::SimulateAll(float aStep)
{
	// perform pending deactivations
	while (!sRemove.empty())
	{
		sAll.erase(sRemove.front());
		sRemove.pop_front();
	}

	// simulate all simulatables
	for (List::iterator itor = sAll.begin(); itor != sAll.end(); ++itor)
		if (*itor)
			(*itor)(aStep);
}
