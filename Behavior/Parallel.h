// parallel: 
class Parallel : public Task
{
public:
	Scheduler &mScheduler;
	std::vector<Task *> mChildren;
	int mMaster;
	bool mFailOnOne;
	bool mCompleteOnOne;
	size_t mFailedCount;
	size_t mCompletedCount;

	Parallel(Scheduler &aScheduler)
		: mScheduler(aScheduler)
		, mMaster(-1)
		, mFailOnOne(false)
		, mCompleteOnOne(false)
		, mFailedCount(0)
		, mCompletedCount(0)
	{
		bind(this, &Parallel::Execute);
	}

	Status Execute(void)
	{
		// clear counts
		mFailedCount = 0;
		mCompletedCount = 0;

		// run all children
		for (size_t i = 0; i < mChildren.size(); ++i)
		{
			mScheduler.Run(*mChildren[i], TaskObserver(this, i == mMaster ? &Parallel::MasterDone : &Parallel::ChildDone));
		}

		// wait for response
		return suspendedTask;
	}

	void MasterDone(Status aStatus)
	{
		// propagate status
		mScheduler.Resume(*this, aStatus);
	}

	void ChildDone(Status aStatus)
	{
		// TO DO: what happens if the task ends in a conflicted state?

		// if child completed...
		if (aStatus == completedTask)
		{
			// if completing on first child, or all children completed
			++mCompletedCount;
			if (mCompleteOnOne || mCompletedCount >= mChildren.size())
			{
				// completed
				mScheduler.Resume(*this, completedTask);
			}
		}

		// if child failed...
		else if (aStatus == failedTask)
		{
			// if failing on first child, or all children failed
			++mFailedCount;
			if (mFailOnOne || mFailedCount >= mChildren.size())
			{
				// failed
				mScheduler.Resume(*this, failedTask);
			}
		}

		// otherwise...
		else
		{
			// propagate status
			mScheduler.Resume(*this, aStatus);
		}
	}
};
