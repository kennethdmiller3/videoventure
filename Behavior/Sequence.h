#pragma once

#include "Task.h"
#include "Scheduler.h"

// sequence: failure on first child failure (logical "and") ("[ ]")
class Sequence : public Task
{
public:
	Scheduler &mScheduler;
	std::vector<Task *> mChildren;
	size_t mCurrent;

public:
	Sequence(Scheduler &aScheduler)
		: mScheduler(aScheduler)
		, mCurrent(0)
	{
		bind(this, &Sequence::Execute);
	}

	Status Execute(void)
	{
		// start with first child
		mCurrent = 0;
		return QueueChild();
	}

	Status QueueChild(void)
	{
		// if children remaining
		if (mCurrent < mChildren.size())
		{
			// queue the next child
			mScheduler.Run(*mChildren[mCurrent], TaskObserver(this, &Sequence::ChildDone));

			// wait for response
			return suspendedTask;
		}

		// completed
		return completedTask;
	}

	void ChildDone(Status aStatus)
	{
		// if child completed...
		if (aStatus == completedTask)
		{
			// queue next child
			++mCurrent;
			mScheduler.Resume(*this, Task(this, &Sequence::QueueChild));
		}

		// otherwise...
		else
		{
			// propagate status
			mScheduler.Resume(*this, aStatus);
		}
	}
};

