#include "StdAfx.h"
#include "Updatable.h"

static Updatable *sHead;
static Updatable *sTail;
static Updatable *sNext;

Updatable::Updatable(unsigned int aId)
: mId(aId), mNext(NULL), mPrev(NULL), entry()
{
}

Updatable::~Updatable(void)
{
	Deactivate();
}

void Updatable::Activate(void)
{
	if (entry.empty())
	{
		entry.bind(this, &Updatable::Update);
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
	}
}

void Updatable::Deactivate(void)
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

void Updatable::UpdateAll(float aStep)
{
	// update all updatables
	Updatable *itor = sHead;
	while (itor)
	{
		// get the next iterator
		// (in case the entry gets deleted)
		sNext = itor->mNext;

		// perform update
		(itor->entry)(aStep);

		// go to the next iterator
		itor = sNext;
	}
}
