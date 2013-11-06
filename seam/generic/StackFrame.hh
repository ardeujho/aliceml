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

#ifndef __GENERIC__STACK_FRAME_HH__
#define __GENERIC__STACK_FRAME_HH__

#if defined(INTERFACE)
#pragma interface "generic/StackFrame.hh"
#endif

#include "store/Store.hh"
class Worker;

class SeamDll StackFrame {
protected:
  enum {WORKER_POS, BASE_SIZE};

  word UnsafeGetArg(u_int pos) {
    return (reinterpret_cast<word *>(this) - pos)[0];
  }
  void UnsafeInitArg(u_int pos, word value) {
    (reinterpret_cast<word *>(this) - pos)[0] = value;
  }
public:
  // StackFrame Constructors
  void New(Worker *worker) {
    reinterpret_cast<word *>(this)[WORKER_POS] = Store::UnmanagedPointerToWord(worker);
  }
  static void New(StackFrame *frame, u_int size, word wFrame) {
    Block *p    = Store::DirectWordToBlock(wFrame);
    for (u_int i = size; i--;)
      frame->UnsafeInitArg(i, p->GetArg(i));
  }
  // StackFrame Accessors
  static u_int GetBaseSize() {
    return BASE_SIZE;
  }
  u_int GetSize() {
    return BASE_SIZE;
  }
  Worker *GetWorker() {
    word wWorker = reinterpret_cast<word *>(this)[WORKER_POS];
    return static_cast<Worker *>(Store::WordToUnmanagedPointer(wWorker));
  }
  word GetArg(u_int pos) {
    return UnsafeGetArg(BASE_SIZE + pos);
  }
  void InitArg(u_int pos, word value) {
    UnsafeInitArg(BASE_SIZE + pos, value);
  }
  void InitArg(u_int pos, s_int value) {
    InitArg(pos, Store::IntToWord(value));
  }
  void InitArgs(u_int startPos, u_int n, word value) {
    for (u_int i=startPos; i<startPos+n; i++) {
      InitArg(i, value);
    }
  }
  void InitArgs(u_int startPos, u_int n, s_int value) {
    InitArgs(startPos, n, Store::IntToWord(value));
  }
  void ReplaceArg(u_int pos, word value) {
    InitArg(pos, value);
  }
  void ReplaceArg(u_int pos, s_int value) {
    InitArg(pos, value);
  }
  SeamMemberDll word Clone();
};

// to be done: better solution
#define NEW_STACK_FRAME(frame,worker,size) \
  StackFrame *frame = Scheduler::PushFrame(size + StackFrame::GetBaseSize()); \
  frame->New(worker);

#define NEW_THREAD_STACK_FRAME(frame,thread,worker,size) \
  StackFrame *frame = thread->PushFrame(size + StackFrame::GetBaseSize()); \
  frame->New(worker);

#endif
