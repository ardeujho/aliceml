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

#ifndef __GENERIC__UNPICKLER_HH__
#define __GENERIC__UNPICKLER_HH__

#if defined(INTERFACE)
#pragma interface "generic/Unpickler.hh"
#endif

#include "generic/Interpreter.hh"
#include "generic/String.hh"

class Unpickler {
public:
  // Exceptions
  static word Corrupt;

  // Unpickler Static Constructor
  static void Init();
  static void InitExceptions();

  typedef word (*handler)(word);
  static void RegisterHandler(String *name, handler handler);

  // Unpickler Functions
  static Interpreter::Result Unpack(String *string, TaskStack *taskStack);
  static Interpreter::Result Load(String *filename, TaskStack *taskStack);
};

#endif
