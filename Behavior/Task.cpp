#include "StdAfx.h"

#include "Task.h"

// task still processing: call again next tick
Status runningTask()
{
	return Task(runningTask);
}

// task active but waiting: call only when notified to resume
Status suspendedTask()
{
	return Task(suspendedTask);
}

// task finished successfully: do not call again
Status completedTask()
{
	return Task(completedTask);
}

// task finished unsuccessfully: do not call again
Status failedTask()
{
	return Task(failedTask);
}

// task had an error: do not call again; abort search and start again
Status errorTask()
{
	return Task(errorTask);
}

// task aborted from outside: should not be returned by tasks
Status abortedTask()
{
	return Task(abortedTask);
}

// empty task: does nothing
Status nullTask()
{
	return Task(0);
}
