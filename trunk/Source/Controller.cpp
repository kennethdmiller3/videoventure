#include "StdAfx.h"
#include "Controller.h"

Controller::List Controller::sAll;

namespace Database
{
	Typed<Controller *> controller("controller");
}

Controller::Controller(unsigned int aId)
: id(aId)
, entry(sAll.end())
, mMove(0, 0)
, mAim(0, 0)
, mFire(false)
{
	if (id > 0)
	{
		entry = sAll.insert(sAll.end(), Entry(this, &Controller::Control));
	}
}

Controller::~Controller(void)
{
	if (entry != sAll.end())
	{
		sAll.erase(entry);
	}
}

void Controller::ControlAll(float aStep)
{
	// update all controllers
	List::iterator itor = sAll.begin();
	while (itor != sAll.end())
	{
		// get the next iterator
		// (in case the entry gets deleted)
		List::iterator next(itor);
		++next;

		// control
		(*itor)(aStep);

		// go to the next iterator
		itor = next;
	}
}
