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

#ifndef __GENERIC__PUSH_CALL_INTERPRETER_HH__
#define __GENERIC__PUSH_CALL_INTERPRETER_HH__

#if defined(INTERFACE)
#pragma interface "generic/PushCallInterpreter.hh"
#endif

#include "generic/Interpreter.hh"

class PushCallInterpreter : public Interpreter {
private:
  // PushCallInterpreter Constructor
  PushCallInterpreter() : Interpreter() {}
public:
  // Exported PushCallInterpreter Instance
  static PushCallInterpreter *self;
  // PushCallInterpreter Static Constructor
  static void Init() {
    self = new PushCallInterpreter();
  }
  // Frame Handling
  static void PushFrame(TaskStack *taskStack, word closure);
  // Execution
  virtual Result Run(TaskStack *taskStack);
  // Debugging
  virtual const char *Identify();
  virtual void DumpFrame(word frame);
};

#endif
