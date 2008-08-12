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
, mActive(false)
, mAction()
, mMove(0, 0)
, mTurn(0)
{
	memset(mFire, 0, sizeof(mFire));
}

Controller::~Controller(void)
{
	Deactivate();
}

void Controller::Activate(void)
{
	if (!mActive)
	{
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
		mActive = true;
	}
}

void Controller::Deactivate(void)
{
	if (mActive)
	{
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
		mActive = false;
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
		(itor->mAction)(aStep);

		// go to the next iterator
		itor = sNext;
	}
}
