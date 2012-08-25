#pragma once

#include "Task.h"

// based on Game::AI++ by Alex J. Champandard

//
// SCHEDULER
//
class Scheduler
{
public:
	// GAME INTERFACE

	// process all active tasks
    void Update(void);

	// execute the next task in the queue
	bool Step(void);

	// abort any remaining tasks
	void Stop(void);


	// TASK MANAGEMENT

	// schedule execution of a new task
    bool Run(Task &aTask, TaskObserver aObserver = TaskObserver(), Task aNext = runningTask);

	// suspend execution of a task, but leave it active
	bool Suspend(Task &aTask);

	// resume execution of a task
	bool Resume(Task &aTask, Task aNext = runningTask);

	// schedule a task to halt
    bool Halt(Task &aTask, TaskObserver aObserver = TaskObserver());

	// attach an observer to a task
	bool SetObserver(Task &aTask, TaskObserver &aObserver);

	// get the current status of a task
	Task GetCurrent(Task &aTask);

	// get the number of active tasks
	size_t GetCount();


	// LIFE CYCLE

	// constructor
	Scheduler(void);

	// set global task termination observer
	void SetTerminationObserver(TaskObserver aObserver)
	{
		mOnTerminate = aObserver;
	}

	// set global task removal observer
	void SetRemovalObserver(TaskObserver aObserver)
	{
		mOnRemove = aObserver;
	}

private:
	// task entry
	struct Entry
	{
		const Task *mTask;		// original scheduled task
		Task mCurrent;			// current task or status
		TaskObserver mInternal;	// observer for task use
		TaskObserver mExternal;	// observer for owner use
		bool mProcessing;		// status recently changed
		int mPriority;			// task priority level

		static bool Activate(Entry *aEntry, Task aNext);
		static bool Deactivate(Entry *aEntry, TaskObserver aObserver = TaskObserver());
	};

	// global task observers
	TaskObserver mOnTerminate;
	TaskObserver mOnRemove;

	// notify observers
	void Notify(Entry &aEntry);

	// insert a task
	void Insert(const Entry &aEntry);

	// task entries
	typedef std::deque<Entry> Entries;
	Entries mEntries;

	// find a task entry
	Entries::iterator FindEvent(const Task &aTask);
	Entry *Find(const Task &aTask);
};
