//
// Author:
//   Leif Kornstaedt <kornstae@ps.uni-sb.de>
//
// Copyright:
//   Leif Kornstaedt, 2002
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#ifndef __EMULATOR__IO_HANDLER_HH__
#define __EMULATOR__IO_HANDLER_HH__

#if defined(INTERFACE)
#pragma interface "emulator/IOHandler.hh"
#endif

#include "emulator/Transients.hh"

class IOHandler {
public:
  static void Init();
  static void Poll();
  static void Block();

  // These return INVALID_POINTER if the fd is already readable/writable:
  static Future *SignalReadable(int fd);
  static Future *SignalWritable(int fd);
};

#endif
