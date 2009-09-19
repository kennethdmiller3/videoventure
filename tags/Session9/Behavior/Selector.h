// selector: success on first child success (logical "or") ("(?)")
class Selector : public Task
{
public:
	Scheduler &mScheduler;
	std::vector<Task *> mChildren;
	size_t mCurrent;

public:
	Selector(Scheduler &aScheduler)
		: mScheduler(aScheduler)
		, mCurrent(-1)
	{
		bind(this, &Selector::Execute);
	}

	Status Execute(void)
	{
		// start with first child
		mCurrent = 0;
		return QueueChild();
	}

	Status QueueChild(void)
	{
		// if children remaining...
		if (mCurrent < mChildren.size())
		{
			// queue the next child
			mScheduler.Run(*mChildren[mCurrent], TaskObserver(this, &Selector::ChildDone));

			// wait for response
			return suspendedTask();
		}

		// failed
		return failedTask();
	}

	void ChildDone(Status aStatus)
	{
		// if child failed...
		if (aStatus == failedTask())
		{
			// queue next child
			++mCurrent;
			mScheduler.Resume(*this, Task(this, &Selector::QueueChild));
		}

		// otherwise...
		else
		{
			// propagate status
			mScheduler.Resume(*this, aStatus);
		}
	}
};
