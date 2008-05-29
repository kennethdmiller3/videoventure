#include "StdAfx.h"
#include "Simulatable.h"

// list of all simulatables
static Simulatable *sHead;
static Simulatable *sTail;
static Simulatable *sNext;

Simulatable::Simulatable(unsigned int aId)
: mId(aId), mNext(NULL), mPrev(NULL), entry()
{
}

Simulatable::~Simulatable(void)
{
	Deactivate();
}

void Simulatable::Activate(void)
{
	if (entry.empty())
	{
		entry.bind(this, &Simulatable::Simulate);
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
	}
}

void Simulatable::Deactivate(void)
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
		(itor->entry)(aStep);

		// go to the next iterator
		itor = sNext;
	}
}
