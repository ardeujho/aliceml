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

#ifndef __ALICE__TYPES_HH__
#define __ALICE__TYPES_HH__

#if defined(INTERFACE)
#pragma interface "alice/Types.hh"
#endif

class Types {
public:
  // Bool.bool
  enum { _false, _true };
  // General.order
  enum { EQUAL, GREATER, LESS };
  // Option.option
  enum { NONE, SOME };
  // List.list
  enum { cons, nil };
  // Future.status
  enum { DETERMINED, FAILED, FUTURE };
  // Component.component
  enum { EVALUATED, UNEVALUATED };
  enum { inf, mod };
  // Config.platform
  enum { UNIX, WIN32 };
  // Label.lab
  enum { ALPHA, NUM };
  // Name.name
  enum { ExId };
};

#endif
