#include "StdAfx.h"

#include "Task.h"

// task still processing: call again next tick
Status RunningTask(void) { return runningTask; }
Status runningTask(RunningTask);

// task active but waiting: call only when notified to resume
Status SuspendedTask(void) { return suspendedTask; }
Status suspendedTask(SuspendedTask);

// task finished successfully: do not call again
Status CompletedTask(void) { return completedTask; }
Status completedTask(CompletedTask);

// task finished unsuccessfully: do not call again
Status FailedTask(void) { return failedTask; }
Status failedTask(FailedTask);

// task had an error: do not call again; abort search and start again
Status ErrorTask(void) { return errorTask; }
Status errorTask(ErrorTask);

// task aborted from outside: should not be returned by tasks
Status AbortedTask(void) { return abortedTask; }
Status abortedTask(AbortedTask);

// empty task: does nothing
Status nullTask(NULL);
