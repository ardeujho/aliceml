//
// Authors:
//   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
//   Leif Kornstaedt <kornstae@ps.uni-sb.de>
//
// Copyright:
//   Thorsten Brunklaus, 2002
//   Leif Kornstaedt, 2002
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#ifndef __GENERIC__SCHEDULER_HH__
#define __GENERIC__SCHEDULER_HH__

#if defined(INTERFACE)
#pragma interface "emulator/Scheduler.hh"
#endif

#include "emulator/Thread.hh"
#include "emulator/ThreadQueue.hh"
#include "emulator/PushCallInterpreter.hh"

class Backtrace;

class Scheduler {
private:
  static word root;
  static ThreadQueue *threadQueue;
  static Thread *currentThread;
  static bool preempt;

  static void Timer();
public:
  static const u_int maxArgs = 16;
  static const u_int ONE_ARG = maxArgs + 1;

  // Scheduler public data
  static u_int nArgs;                 // Number of arguments
  static word currentArgs[maxArgs];   // Arguments
  static word currentData;            // Transient or Exception
  static Backtrace *currentBacktrace; // Backtrace
  static word vmGUID;
  // Scheduler Static Constructor
  static void Init();

  // Scheduler Main Function
  static void Run();

  // Scheduler Accessors
  static Thread *GetCurrentThread() {
    return currentThread;
  }
  // Scheduler Functions
  static void NewThread(u_int nArgs, word args, TaskStack *taskStack) {
    Thread *thread = Thread::New(nArgs, args, taskStack);
    threadQueue->Enqueue(thread);
  }
  static void NewThread(word closure, u_int nArgs, word args,
			TaskStack *taskStack) {
    PushCallInterpreter::PushFrame(taskStack, closure);
    NewThread(nArgs, args, taskStack);
  }
  static void ScheduleThread(Thread *thread) {
    //--** precondition: must not be scheduled
    Assert(thread->GetState() == Thread::RUNNABLE);
    threadQueue->Enqueue(thread);
  }
  static void WakeupThread(Thread *thread) {
    Assert(thread->GetState() == Thread::BLOCKED);
    thread->Wakeup();
    if (!thread->IsSuspended())
      ScheduleThread(thread);
  }
  static void SuspendThread(Thread *thread) {
    thread->Suspend();
    thread->GetTaskStack()->Purge();
    if (thread->GetState() == Thread::RUNNABLE)
      threadQueue->Remove(thread);
  }
  static void ResumeThread(Thread *thread) {
    if (thread->IsSuspended()) {
      thread->Resume();
      if (thread->GetState() == Thread::RUNNABLE)
	ScheduleThread(thread);
    }
  }
  static bool TestPreempt() {
    return preempt;
  }
};

#endif
