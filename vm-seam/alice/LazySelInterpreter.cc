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

#if defined(INTERFACE)
#pragma implementation "alice/LazySelInterpreter.hh"
#endif

#include <cstdio>
#include "alice/Data.hh"
#include "alice/LazySelInterpreter.hh"

// LazySel Frame
class LazySelFrame: private StackFrame {
private:
  enum { RECORD_POS, LABEL_POS, SIZE };
public:
  static LazySelFrame *New(Interpreter *interpreter, word record,
			   UniqueString *label) {
    NEW_STACK_FRAME(frame, interpreter, SIZE);
    frame->InitArg(RECORD_POS, record);
    frame->InitArg(LABEL_POS, label->ToWord());
    return STATIC_CAST(LazySelFrame *, frame);
  }

  u_int GetSize() {
    return StackFrame::GetSize() + SIZE;
  }
  word GetRecord() {
    return GetArg(RECORD_POS);
  }
  UniqueString *GetLabel() {
    return UniqueString::FromWordDirect(GetArg(LABEL_POS));
  }
};

//
// LazySelInterpreter
//
LazySelInterpreter *LazySelInterpreter::self;

void LazySelInterpreter::Init() {
  self = new LazySelInterpreter();
}

void LazySelInterpreter::PushCall(Closure *closure0) {
  LazySelClosure *closure = STATIC_CAST(LazySelClosure *, closure0);
  LazySelFrame::New(self, closure->GetRecord(), closure->GetLabel());
}

u_int LazySelInterpreter::GetFrameSize(StackFrame *sFrame) {
  LazySelFrame *frame = STATIC_CAST(LazySelFrame *, sFrame);
  Assert(sFrame->GetWorker() == this);
  return frame->GetSize();
}

Worker::Result LazySelInterpreter::Run(StackFrame *sFrame) {
  LazySelFrame *frame = STATIC_CAST(LazySelFrame *, sFrame);
  Assert(sFrame->GetWorker() == this);
  word wRecord = frame->GetRecord();
  Transient *transient = Store::WordToTransient(wRecord);
  if (transient == INVALID_POINTER) { // is determined
    UniqueString *label = frame->GetLabel();
    Scheduler::PopFrame(frame->GetSize());
    Scheduler::nArgs = 1;
    Scheduler::currentArgs[0] = Record::FromWord(wRecord)->PolySel(label);
    return Worker::CONTINUE;
  } else { // need to request
    Scheduler::currentData = wRecord;
    return Worker::REQUEST;
  }
}

u_int LazySelInterpreter::GetInArity(ConcreteCode *) {
  return 0;
}

u_int LazySelInterpreter::GetOutArity(ConcreteCode *) {
  return INVALID_INT; // TODO: Correct arity needed?
}

const char *LazySelInterpreter::Identify() {
  return "LazySelInterpreter";
}

void LazySelInterpreter::DumpFrame(StackFrame *sFrame) {
  LazySelFrame *frame = STATIC_CAST(LazySelFrame *, sFrame);
  Assert(sFrame->GetWorker() == this);
  std::fprintf(stderr, "Select %s\n", frame->GetLabel()->ToString()->ExportC());
}
