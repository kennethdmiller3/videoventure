#include "StdAfx.h"
#include "Controller.h"

namespace Database
{
	Typed<Controller *> controller(0xb4652c81 /* "controller" */);
}

// list of all controllers
static Controller *sHead;
static Controller *sTail;
static Controller *sNext;

Controller::Controller(unsigned int aId)
: mId(aId)
, mNext(NULL)
, mPrev(NULL)
, entry()
, mMove(0, 0)
, mAim(0, 0)
{
	memset(mFire, 0, sizeof(mFire));
}

Controller::~Controller(void)
{
	Deactivate();
}

void Controller::Activate(void)
{
	if (entry.empty())
	{
		entry.bind(this, &Controller::Control);
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
	}
}

void Controller::Deactivate(void)
{
	if (!entry.empty())
	{
		entry.clear();
		if (sHead == this)
			sHead = mNext;
		if (sTail == this)
			sTail = mPrev;
		if (sNext == this)
			sNext = mNext;
		if (mNext)
			mNext->mPrev = mPrev;
		if (mPrev)
			mPrev->mNext = mNext;
		mNext = NULL;
		mPrev = NULL;
	}
}

void Controller::ControlAll(float aStep)
{
	// update all controllers
	Controller *itor = sHead;
	while (itor)
	{
		// get the next iterator
		// (in case the entry gets deleted)
		sNext = itor->mNext;

		// perform simulation
		(itor->entry)(aStep);

		// go to the next iterator
		itor = sNext;
	}
}
