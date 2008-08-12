#include "StdAfx.h"
#include "Updatable.h"

static Updatable *sHead;
static Updatable *sTail;
static Updatable *sNext;

Updatable::Updatable(unsigned int aId)
: mId(aId)
, mNext(NULL)
, mPrev(NULL)
, mActive(false)
, mAction()
{
}

Updatable::~Updatable(void)
{
	Deactivate();
}

void Updatable::Activate(void)
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

void Updatable::Deactivate(void)
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

void Updatable::UpdateAll(float aStep)
{
	// update all updatables
	Updatable *itor = sHead;
	while (itor)
	{
		// get the next iterator
		// (in case the entry gets deleted)
		sNext = itor->mNext;

		// perform action
		(itor->mAction)(aStep);

		// go to the next iterator
		itor = sNext;
	}
}
