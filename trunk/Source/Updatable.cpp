#include "StdAfx.h"
#include "Updatable.h"

Updatable::List Updatable::sAll;
Updatable::Remove Updatable::sRemove;

Updatable::Updatable(unsigned int aId)
: id(aId), entry(sAll.end())
{
	if (id > 0)
		Activate();
}

Updatable::~Updatable(void)
{
	Deactivate();
}

void Updatable::Activate(void)
{
	if (entry == sAll.end())
	{
		entry = sAll.insert(sAll.end(), Entry(this, &Updatable::Update));
	}
}

void Updatable::Deactivate(void)
{
	if (entry != sAll.end())
	{
		sRemove.push_back(entry);
		entry->clear();
		entry = sAll.end();
	}
}

void Updatable::UpdateAll(float aStep)
{
	// perform pending deactivations
	while (!sRemove.empty())
	{
		sAll.erase(sRemove.front());
		sRemove.pop_front();
	}

	// update all updatables
	for (List::iterator itor = sAll.begin(); itor != sAll.end(); ++itor)
		if (*itor)
			(*itor)(aStep);
}
