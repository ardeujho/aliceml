//
// Authors:
//   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
//
// Copyright:
//   Thorsten Brunklaus, 2002
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#if defined(INTERFACE)
#pragma implementation "generic/PushCallWorker.hh"
#endif

#include <cstdio>
#include "generic/Scheduler.hh"
#include "generic/StackFrame.hh"
#include "generic/PushCallWorker.hh"

// PushCall Frame
class PushCallFrame: private StackFrame {
private:
  enum { CLOSURE_POS, SIZE };
public:
  using Block::ToWord;

  // PushCallFrame Constructor
  static PushCallFrame *New(Worker *worker, word closure) {
    StackFrame *frame = StackFrame::New(PUSHCALL_FRAME, worker, SIZE);
    frame->InitArg(CLOSURE_POS, closure);
    return static_cast<PushCallFrame *>(frame);
  }
  // PushCallFrame Untagging
  static PushCallFrame *FromWordDirect(word frame) {
    StackFrame *p = StackFrame::FromWordDirect(frame);
    Assert(p->GetLabel() == PUSHCALL_FRAME);
    return static_cast<PushCallFrame *>(p);
  }

  // PushCallFrame Accessors
  word GetClosure() {
    return StackFrame::GetArg(CLOSURE_POS);
  }
};

//
// PushCallWorker Functions
//
PushCallWorker *PushCallWorker::self;

void PushCallWorker::PushFrame(word closure) {
  Scheduler::PushFrame(PushCallFrame::New(self, closure)->ToWord());
}

void PushCallWorker::PushFrame(Thread *thread, word closure) {
  thread->PushFrame(PushCallFrame::New(self, closure)->ToWord());
}

Worker::Result PushCallWorker::Run() {
  PushCallFrame *frame =
    PushCallFrame::FromWordDirect(Scheduler::GetAndPopFrame());
  return Scheduler::PushCall(frame->GetClosure());
}

const char *PushCallWorker::Identify() {
  return "PushCallWorker";
}

void PushCallWorker::DumpFrame(word) {
  std::fprintf(stderr, "Push Call\n");
}
