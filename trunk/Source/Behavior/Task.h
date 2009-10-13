#pragma once

// based on Game::AI++ by Alex J. Champandard

// task
// > condition
// > > instant (return FAILED or COMPLETED)
// > > continuous (return FAILED or RUNNING)
// > action
struct Task;
typedef fastdelegate::FastDelegate<Task (void)> TaskDelegate;
struct Task : public TaskDelegate
{
	// default/copy constructor
    Task(TaskDelegate aDelegate = 0)
		:  TaskDelegate(aDelegate)
	{
	}
    
    // forward to delegate constructor
    template <typename H, typename F> Task(H aHolder, F aFunc)
		: TaskDelegate(aHolder, aFunc)
	{
	}
};

// status task
typedef Task Status;

// task observer (callback)
typedef fastdelegate::FastDelegate<void (Status)> TaskObserver;

// task still processing: call again next tick
extern GAME_API Status runningTask;

// task active but waiting: call only when notified to resume
extern GAME_API Status suspendedTask;

// task finished successfully: do not call again
extern GAME_API Status completedTask;

// task finished unsuccessfully: do not call again
extern GAME_API Status failedTask;

// task had an error: do not call again; abort search and start again
extern GAME_API Status errorTask;

// task aborted from outside: should not be returned by tasks
extern GAME_API Status abortedTask;

// empty task: does nothing
extern GAME_API Status nullTask;

// is the specified task active?
inline bool IsTaskActive(const Task &aStatus)
{
	return
		aStatus != completedTask &&
		aStatus != failedTask &&
		aStatus != errorTask &&
		aStatus != abortedTask;
}
