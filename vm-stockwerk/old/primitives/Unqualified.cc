//
// Authors:
//   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
//   Leif Kornstaedt <kornstae@ps.uni-sb.de>
//
// Copyright:
//   Thorsten Brunklaus, 2000
//   Leif Kornstaedt, 2000
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#include "alice/primitives/Authoring.hh"

static TaskManager::Result Compare(TaskStack *taskStack, u_int frameSize,
				   word x0, word x1) {
  Block *a = Store::WordToBlock(x0);
  Block *b = Store::WordToBlock(x1);
  if (a == INVALID_POINTER) { // a is Transient or int
    DECLARE_INT(i, x0);
    if (b != INVALID_POINTER) { // b is TagVal
      RETURN_BOOL(false);
    }
    DECLARE_INT(j, x1);
    RETURN_BOOL(i == j);
  }
  if (b == INVALID_POINTER) { // b is Transient or int, but a is a block
    DECLARE_INT(j, x1);
    RETURN_BOOL(false);
  }
  // from here, both a and b are blocks
  BlockLabel label = a->GetLabel();
  switch (label) {
  case TUPLE_LABEL:
  case Alice::ConVal:
  case Alice::Vector:
  case Alice::VectorZero:
    {
      u_int size = a->GetSize();
      if (b->GetSize() != size)
	RETURN_BOOL(false);
      for (u_int i = 1; i <= size; i++) {
	TaskManager::Result result =
	  Compare(taskStack, frameSize, a->GetArg(i), b->GetArg(i));
	if (result.code == TaskManager::Result::CONTINUE) {
	  bool b = taskStack->GetInt(0);
	  taskStack->PopFrame(1);
	  if (!b) RETURN_BOOL(false);
	} else {
	  return result;
	}
      }
    }
    RETURN_BOOL(true);
  case CHUNK_LABEL:
    {
      u_int size = a->GetSize();
      if (b->GetSize() != size)
	RETURN_BOOL(false);
      for (u_int i = 1; i <= size; i++)
	if (a->GetArg(i) != b->GetArg(i))
	  RETURN_BOOL(false);
      RETURN_BOOL(true);
    }
  default:
    if (label == b->GetLabel()) {
      RETURN_BOOL(a == b);
    }
  }
  RETURN_BOOL(false);
}

DEFINE2(opeq) {
  return Compare(taskStack, frameSize, x0, x1);
} END

DEFINE2(opnoteq) { // NON-ABSTRACT TASK STACK USE
  TaskManager::Result result = Compare(taskStack, frameSize, x0, x1);
  if (result.code == TaskManager::Result::CONTINUE)
    taskStack->PutInt(0, !taskStack->GetInt(0));
  return result;
} END

void PrimitiveTable::RegisterUnqualified() {
  Register("=", opeq, 2);
  Register("<>", opnoteq, 2);
}
