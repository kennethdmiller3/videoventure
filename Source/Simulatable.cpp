#include "StdAfx.h"
#include "Simulatable.h"

// list of all simulatables
static Simulatable *sHead;
static Simulatable *sTail;
static Simulatable *sNext;

Simulatable::Simulatable(unsigned int aId)
: mId(aId)
, mNext(NULL)
, mPrev(NULL)
, mActive(false)
, mAction()
{
}

Simulatable::~Simulatable(void)
{
	Deactivate();
}

void Simulatable::Activate(void)
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

void Simulatable::Deactivate(void)
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

void Simulatable::SimulateAll(float aStep)
{
	// simulate all simulatables
	Simulatable *itor = sHead;
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
