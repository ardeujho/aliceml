//
// Author:
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
#pragma implementation "store/JITStore.hh"
#endif

#include "store/JITStore.hh"

// This is the only instance of lightning state
jit_state lightning;

//
// Logging Helper Functions
//
static FILE *execLog;

static const char *RegToString(u_int Reg) {
  switch (Reg) {
  case JIT_R0:
    return "R0";
  case JIT_R1:
    return "R1";
  case JIT_R2:
    return "R2";
  case JIT_V0:
    return "V0";
  case JIT_V1:
    return "V1";
  case JIT_V2:
    return "V2";
  case JIT_SP:
    return "SP";
  default:
  return "INVALID_REGISTER";
  }
}

static void SaveContext() {
  jit_pushr_ui(JIT_R0);
  jit_pushr_ui(JIT_R1);
  jit_pushr_ui(JIT_R2);
  jit_pushr_ui(JIT_V0);
  jit_pushr_ui(JIT_V1);
  jit_pushr_ui(JIT_V2);
}

static void RestoreContext() {
  jit_popr_ui(JIT_V2);
  jit_popr_ui(JIT_V1);
  jit_popr_ui(JIT_V0);
  jit_popr_ui(JIT_R2);
  jit_popr_ui(JIT_R1);
  jit_popr_ui(JIT_R0);
}

static void ShowMessage(const char *info) {
  fprintf(execLog, info);
  fflush(execLog);
}

static void CompileMessage(const char *info) {
  jit_movi_p(JIT_R0, info);
  jit_pushr_ui(JIT_R0);
  JITStore::Call(1, (void *) ShowMessage);
}

static void ShowRegister(const char *info, word value) {
  fprintf(execLog, info, value);
  fflush(execLog);
}

static void CompileRegister(u_int Reg) {
  static char buffer[256];
  sprintf(buffer, "%s = %sp\n", RegToString(Reg), "%");
  jit_pushr_ui(Reg);
  jit_movi_p(JIT_R0, strdup(buffer));
  jit_pushr_ui(JIT_R0);
  JITStore::Call(2, (void *) ShowRegister);
}

//
// JITStore Functions
//
void JITStore::InitLoggging() {
#if defined(JIT_STORE_DEBUG)
  if ((execLog = fopen("execlog.txt", "w")) == NULL) {
    fprintf(stderr, "unable to open exec log\n");
    exit(1);
  }
#endif
}

void JITStore::LogMesg(const char *info) {
#if defined(JIT_STORE_DEBUG)
  SaveContext();
  CompileMessage(info);
  RestoreContext();
#endif
}

void JITStore::LogReg(u_int Value) {
#if defined(JIT_STORE_DEBUG)
  SaveContext();
  CompileRegister(Value);
  RestoreContext();
#else
  // Avoid compiler warnings
  Value = Value;
#endif
}

void JITStore::LogRead(u_int Dest, u_int Ptr, u_int Index) {
#if defined(JIT_STORE_DEBUG)
  static char buffer[256];
  SaveContext();
  CompileRegister(Ptr);
  sprintf(buffer, "%s <- %s[%d]...",
	  RegToString(Dest), RegToString(Ptr), Index);
  CompileMessage(strdup(buffer));
  RestoreContext();
#else
  // Avoid Compiler warnings
  Dest  = Dest;
  Ptr   = Ptr;
  Index = Index;
#endif
}

void JITStore::LogWrite(u_int Ptr, u_int Index, u_int Value) {
#if defined(JIT_STORE_DEBUG)
  static char buffer[256];
  SaveContext();
  jit_pushr_ui(Value);
  CompileRegister(Ptr);
  jit_popr_ui(Value);
  CompileRegister(Value);
  sprintf(buffer, "%s[%d] <- %s...",
	  RegToString(Ptr), Index, RegToString(Value));
  CompileMessage(strdup(buffer));
  RestoreContext();
#else
  // Avoid compiler warnings
  Ptr   = Ptr;
  Index = Index;
  Value = Value;
#endif
}
