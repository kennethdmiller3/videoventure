#include "StdAfx.h"

#include "Scheduler.h"

// TASK MANAGEMENT

// schedule execution of a new task
bool Scheduler::Run(Task &aTask, TaskObserver aObserver, Task aNext)
{
	if (aNext == runningTask())
		aNext = aTask;

	// build a new entry
	Entry entry = { &aTask, aNext, NULL, aObserver, false, false };

	// if currently updating...
	if (mUpdating)
	{
		// add to front of queued entries
		mQueued.push_front(entry);
		//mQueued.push_back(entry);
	}
	else
	{
		// add to back of active entries
		mActive.push_back(entry);
	}

	// success
	return true;
}

// suspend execution of a task, but leave it active
bool Scheduler::Suspend(Task &aTask)
{
	Entry *entry = Find(aTask);
	if (entry && IsTaskActive(entry->mCurrent))
	{
		entry->mCurrent = suspendedTask();
		Notify(*entry);
		return true;
	}
	return false;
}

// 
// resume execution of a task
bool Scheduler::Resume(Task &aTask, Task aNext)
{
	if (aNext == runningTask())
		aNext = aTask;

	if (!mUpdating)
	{
		Entry *entry = FindActive(aTask);
		return Entry::Activate(entry, aNext);
	}

	Entry *entry = FindQueued(aTask);
	if (entry != NULL)
	{
		return Entry::Activate(entry, aNext);
	}

	entry = FindActive(aTask);
	if (Entry::Activate(entry, aNext))
	{
		mQueued.push_front(*entry);
		mQueued.front().mDuplicate = true;
		return true;
	}

	return false;
}

// schedule a task to halt
bool Scheduler::Halt(Task &aTask)
{
	// if updating...
	if (mUpdating)
	{
		// if task is queued...
		Entry *entry = FindQueued(aTask);
		if (entry)
		{
			// deactivate queued tasks
			return Entry::Deactivate(entry);
		}
	}

	// deactivate in active tasks
	Entry *entry = FindActive(aTask);
	if (!Entry::Deactivate(entry))
		return false;

	// if updating...
	if (mUpdating)
	{
		// queue for immediate processing
		mQueued.push_front(*entry);
		mQueued.front().mDuplicate = true;
	}

	return true;
}

// attach an observer to a task
bool Scheduler::SetObserver(Task &aTask, TaskObserver &aObserver)
{
	Entry *entry = Find(aTask);
	if (entry)
	{
		entry->mInternal = aObserver;
		return true;
	}
	return false;
}

// get the current status of a task
Task Scheduler::GetCurrent(Task &aTask)
{
	Entry *entry = Find(aTask);
	return entry ? entry->mCurrent : nullTask();
}

// get the number of active tasks
size_t Scheduler::GetCount()
{
	return mActive.size();
}


// GAME INTERFACE

// process all active tasks
void Scheduler::Update()
{
	Start();
	while (Step());
}

// queue tasks to be executed
void Scheduler::Start()
{
	mUpdating = true;

	// clear queued tasks
	mQueued.clear();

	// queue all active (or just-changed) tasks
	for (Active::iterator i = mActive.begin(); i != mActive.end(); ++i)
	{
		if (IsTaskActive(i->mCurrent) || i->mProcessing)
		{
			i->mProcessing = false;
			mQueued.push_back(*i);
		}
	}

	// clear active tasks
	mActive.clear();
}

// execute the next task in the queue
bool Scheduler::Step()
{
	if (mQueued.empty())
	{
		mUpdating = false;
		return false;
	}

	// get next queued entry
	Entry entry = mQueued.front();
	mQueued.pop_front();

	// if not a duplicate...
	if (!entry.mDuplicate)
	{
		assert(!FindActive(*entry.mTask));

		// put back into active tasks
		// (as the task may use the scheduler)
		mActive.push_back(entry);
	}

	// execute task
	// ("status" tasks return themselves)
	Task next = entry.mCurrent();
	if (next != runningTask())
		entry.mCurrent = next;

	// update status
	if (entry.mDuplicate)
	{
		Entry *original = FindActive(*entry.mTask);
		original->mCurrent = entry.mCurrent;
	}
	else
	{
		mActive.back().mCurrent = entry.mCurrent;
	}

	// if not active
	if (!IsTaskActive(entry.mCurrent))
	{
		// notify observers
		Notify(entry);
	}

	return true;
}

// abort any remaining tasks
void Scheduler::Stop()
{
	assert(!mUpdating);
	for (Active::iterator i = mActive.begin(); i != mActive.end(); ++i)
	{
		Entry::Deactivate(&*i);
	}
}


// LIFE CYCLE

// constructor
Scheduler::Scheduler()
: mUpdating(false)
{
}

// set global task observer
void Scheduler::SetObserver(TaskObserver &aObserver)
{
	mObserver = aObserver;
}

Scheduler::Entry *Scheduler::FindActive(const Task &aTask)
{
	for (Active::iterator i = mActive.begin(); i != mActive.end(); ++i)
	{
		if (i->mTask == &aTask)
			return &*i;
	}
	return NULL;
}

Scheduler::Entry *Scheduler::FindQueued(const Task &aTask)
{
	for (Queued::iterator i = mQueued.begin(); i != mQueued.end(); ++i)
	{
		if (i->mTask == &aTask && !i->mDuplicate)
			return &*i;
	}
	return NULL;
}

Scheduler::Entry *Scheduler::Find(const Task &aTask)
{
	if (mUpdating)
	{
		if (Entry *entry = FindQueued(aTask))
			return entry;
	}
	return FindActive(aTask);
}

void Scheduler::Notify(Scheduler::Entry &aEntry)
{
	if (!aEntry.mInternal.empty())
		aEntry.mInternal(aEntry.mCurrent);
	if (!aEntry.mExternal.empty())
		aEntry.mExternal(aEntry.mCurrent);
	if (!mObserver.empty() && aEntry.mCurrent != suspendedTask())
		mObserver(*aEntry.mTask);
}


bool Scheduler::Entry::Activate(Entry *aEntry, Task aNext)
{
	if (aEntry != NULL && aEntry->mCurrent == suspendedTask())
	{
		aEntry->mCurrent = aNext;
		return true;
	}
	return false;
}
bool Scheduler::Entry::Deactivate(Entry *aEntry)
{
	if (aEntry != NULL && IsTaskActive(aEntry->mCurrent))
	{
		aEntry->mCurrent = abortedTask();
		aEntry->mProcessing = true;
		return true;
	}
	return false;
}
