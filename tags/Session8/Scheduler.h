#pragma once

#include "Task.h"

// scheduler
class Scheduler
{
public:
	// TASK MANAGEMENT

	// schedule execution of a new task
    bool Run(Task &aTask, TaskObserver aObserver = TaskObserver(), Task aNext = runningTask());

	// suspend execution of a task, but leave it active
	bool Suspend(Task &aTask);

	// resume execution of a task
	bool Resume(Task &aTask, Task aNext = runningTask());

	// schedule a task to halt
    bool Halt(Task &aTask);

	// attach an observer to a task
	bool SetObserver(Task &aTask, TaskObserver &aObserver);

	// get the current status of a task
	Task GetCurrent(Task &aTask);

	// get the number of active tasks
	size_t GetCount();


	// GAME INTERFACE

	// process all active tasks
    void Update(void);

	// queue tasks to be executed
	void Start(void);

	// execute the next task in the queue
	bool Step(void);

	// abort any remaining tasks
	void Stop(void);


	// LIFE CYCLE

	// constructor
	Scheduler(void);

	// set global task observer
	void SetObserver(TaskObserver &aObserver);

private:
	// global task observer
	TaskObserver mObserver;

	// task entry
	struct Entry
	{
		const Task *mTask;		// original scheduled task
		Task mCurrent;			// current task or status
		TaskObserver mInternal;	// observer for task use
		TaskObserver mExternal;	// observer for owner use
		bool mProcessing;		// status changed
		bool mDuplicate;		// is a duplicate

		static bool Activate(Entry *aEntry, Task aNext);
		static bool Deactivate(Entry *aEntry);
	};

	// active tasks
	typedef std::vector<Entry> Active;
	Active mActive;

	// queued tasks
	typedef std::deque<Entry> Queued;
	Queued mQueued;

	// currently updating?
	bool mUpdating;

	// find task in the active list
	Entry *FindActive(const Task &aTask);
	
	// find task in the working queue
	Entry *FindQueued(const Task &aTask);

	// find task
	Entry *Find(const Task &aTask);

	// notify observers
	void Notify(Entry &aEntry);
};
