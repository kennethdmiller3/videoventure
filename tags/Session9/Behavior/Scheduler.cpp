#include "StdAfx.h"

#include "Scheduler.h"

#include <algorithm>


// TASK MANAGEMENT

// schedule execution of a new task
bool Scheduler::Run(Task &aTask, TaskObserver aObserver, Task aNext)
{
	if (aNext == runningTask())
		aNext = aTask;

	// build a new entry
	Entry entry = { &aTask, aNext, NULL, aObserver, false, 0 };

	// add to entry queue
	mEntries.push_back(entry);
	SortQueue();

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

	Entries::iterator it = FindEvent(aTask);
	if (it == mEntries.end())
	{
		return false;
	}

	Entry entry = *it;
	if (Entry::Activate(&entry, aNext))
	{
		mEntries.erase(it);
		mEntries.push_back(entry);
		SortQueue();
		return true;
	}

	return false;
}

// schedule a task to halt
bool Scheduler::Halt(Task &aTask, TaskObserver aObserver)
{
	Entries::iterator it = FindEvent(aTask);
	if (it == mEntries.end())
	{
		return false;
	}

	Entry entry = *it;
	if (Entry::Deactivate(&entry, aObserver))
	{
		mEntries.erase(it);
		mEntries.push_back(entry);
		SortQueue();
		return true;
	}

	return false;
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
	// deduct one for the boundary marker
	return mEntries.size() - 1;
}


// GAME INTERFACE

// process all active tasks
void Scheduler::Update()
{
	while (Step())
	{
		continue;
	}
}

// execute the next task in the queue
bool Scheduler::Step()
{
	// get the next item in the queue
	Entry e = mEntries.back();
	mEntries.pop_back();
	
	// if the item is a marker
	if (e.mTask == NULL)
	{
		// stop the current update
		mEntries.push_front(e);
		SortQueue();
		return false;
	}

	// if the task is not active...
	if (!IsTaskActive(e.mCurrent))
	{
		// if processing...
		if (e.mProcessing)
		{
			// notify observers and hold for next update
			Notify(e);
			e.mProcessing = false;
			mEntries.push_front(e);
		}
		else
		{
			// notify removal observer
			if (mOnRemove)
				mOnRemove(*e.mTask);
		}
		return true;
	}
	e.mProcessing = false;
	
	// put the entry into the queue to make it available for the scheduler
	mEntries.push_front(e);
	
	// run the task
	// (status tasks return themselves)
	Task next = e.mCurrent();

	// update current task
	if (next != runningTask())
	{
		e.mCurrent = next;
	}

	// rescan from front entry as it may have changed
	Entries::iterator it = mEntries.begin();
	for (; it != mEntries.end(); ++it)
	{
		if (it->mTask == e.mTask)
		{
			break;
		}
	}
	
	if (it->mProcessing)
	{
		return true;
	}
	
	// update current status
	it->mCurrent = e.mCurrent;
	
	// if not active
	if (!IsTaskActive(e.mCurrent))
	{
		// notify observers
		Notify(e);
	}

	return true;
}

// abort any remaining tasks
void Scheduler::Stop()
{
	for (Entries::iterator i = mEntries.begin(); i != mEntries.end(); ++i)
	{
		Entry::Deactivate(&*i);
	}
}

// sort tasks
void Scheduler::SortQueue()
{
	Entries::iterator i = mEntries.begin();

	// find the boundary marker
	while (i->mTask != NULL)
	{
		++i;
	}

	// sort entries past the marker
	stable_sort(++i, mEntries.end());
}


// LIFE CYCLE

// constructor
Scheduler::Scheduler()
{
	// boundary marker separating current and next update
	Entry marker = { NULL, nullTask(), NULL, NULL, false, 0 };
	mEntries.push_back(marker);
}

// find
Scheduler::Entry *Scheduler::Find(const Task &aTask)
{
	Entries::iterator i = FindEvent(aTask);
	if (i == mEntries.end())
	{
		return NULL;
	}
	return &*i;
}

// find event
Scheduler::Entries::iterator Scheduler::FindEvent(const Task &task)
{
	Entries::iterator i;
	for (i = mEntries.begin(); i != mEntries.end(); ++i)
	{
		if (i->mTask == &task)
		{
			return i;
		}
	}
	return mEntries.end();
}

// notify observers
void Scheduler::Notify(Scheduler::Entry &aEntry)
{
	if (!aEntry.mInternal.empty())
		aEntry.mInternal(aEntry.mCurrent);
	if (!aEntry.mExternal.empty())
		aEntry.mExternal(aEntry.mCurrent == abortedTask() ? failedTask() : aEntry.mCurrent);
	if (!mOnTerminate.empty() && aEntry.mCurrent != suspendedTask())
		mOnTerminate(*aEntry.mTask);
}

// activate an entry
bool Scheduler::Entry::Activate(Entry *aEntry, Task aNext)
{
	if (aEntry != NULL && aEntry->mCurrent == suspendedTask())
	{
		aEntry->mCurrent = aNext;
		aEntry->mProcessing = true;
		return true;
	}
	return false;
}

// deactivate an entry
bool Scheduler::Entry::Deactivate(Entry *aEntry, TaskObserver aObserver)
{
	if (aEntry == NULL)
		return false;

	if (aEntry->mExternal == aObserver)
		aEntry->mExternal.clear();

	if (IsTaskActive(aEntry->mCurrent))
	{
		aEntry->mCurrent = abortedTask();
		aEntry->mProcessing = true;
		aEntry->mPriority = 1;
		return true;
	}
	return false;
}
