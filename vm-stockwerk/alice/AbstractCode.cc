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

#if defined(INTERFACE)
#pragma implementation "alice/AbstractCode.hh"
#endif

#include "adt/Stack.hh"
#include "adt/Queue.hh"
#include "generic/Tuple.hh"
#include "generic/Debug.hh"
#include "alice/AbstractCode.hh"

static const char *opcodes[AbstractCode::nInstrs] = {
  "AppPrim", "AppVar", "Close", "CompactIntTest", "CompactTagTest", "ConTest",
  "DirectAppVar", "EndHandle", "EndTry", "GetRef", "GetTup", "IntTest", "Kill",
  "LazyPolySel", "PutCon", "PutNew", "PutRef", "PutTag", "PutTup",
  "PutPolyRec", "PutVar", "PutVec", "Raise", "RealTest", "Reraise",
  "Return", "Sel", "Shared", "Specialize", "StringTest", "TagTest",
  "Try", "VecTest"
};

#define OPT(w, X) {				\
  TagVal *opt = TagVal::FromWord(w);		\
  if (opt == INVALID_POINTER)			\
    fprintf(file, " NONE");			\
  else {					\
    fprintf(file, " SOME");			\
    X(opt->Sel(0));				\
  }						\
}

#define TUPLE(w, X, Y) {			\
  Tuple *tuple = Tuple::FromWordDirect(w);	\
  fprintf(file, " (");				\
  X(tuple->Sel(0));				\
  Y(tuple->Sel(1));				\
  fprintf(file, " )");				\
}

#define TRIPLE(w, X, Y, Z) {			\
  Tuple *tuple = Tuple::FromWordDirect(w);	\
  fprintf(file, " (");				\
  X(tuple->Sel(0));				\
  Y(tuple->Sel(1));				\
  Z(tuple->Sel(2));				\
  fprintf(file, " )");				\
}

#define VECTOR(w, X) {					\
  Vector *vector = Vector::FromWordDirect(w);		\
  fprintf(file, " %d#[", vector->GetLength());		\
  for (u_int i = 0; i < vector->GetLength(); i++) {	\
    X(vector->Sub(i));					\
  }							\
  fprintf(file, " ]");					\
}

#define ARGS(w, X) {				\
  TagVal *args = TagVal::FromWordDirect(w);	\
  switch (AbstractCode::GetArgs(args)) {	\
  case AbstractCode::OneArg:			\
    X(args->Sel(0));				\
    break;					\
  case AbstractCode::TupArgs:			\
    VECTOR(args->Sel(0), X);			\
    break;					\
  }						\
}

#define INSTR             Instr(pc->Sel(operand++));
#define LASTINSTR         LastInstr(pc->Sel(operand++));
#define INSTRS            VECTOR(pc->Sel(operand++), Instr)
#define INT               Int(pc->Sel(operand++));
#define STRING            Value(pc->Sel(operand++));
#define VALUE             Value(pc->Sel(operand++));
#define TEMPLATE          Template(pc->Sel(operand++));
#define STAMP             Int(pc->Sel(operand++));
#define ID                Int(pc->Sel(operand++));
#define IDS               VECTOR(pc->Sel(operand++), Int)
#define IDREF             IdRef(pc->Sel(operand++));
#define IDREFS            VECTOR(pc->Sel(operand++), IdRef)
#define IDREFARGS         ARGS(pc->Sel(operand++), IdRef)
#define IDDEF             IdDef(pc->Sel(operand++));
#define IDDEFS            VECTOR(pc->Sel(operand++), IdDef)
#define IDDEFINSTROPT     OPT(pc->Sel(operand++), IdDefInstr)
#define IDDEFARGSINSTROPT OPT(pc->Sel(operand++), IdDefArgsInstr)
#define LABEL             Label(pc->Sel(operand++));
#define LABELS            VECTOR(pc->Sel(operand++), Label)
#define INTINSTRVEC       VECTOR(pc->Sel(operand++), IntInstr)
#define REALINSTRVEC      VECTOR(pc->Sel(operand++), RealInstr)
#define STRINGINSTRVEC    VECTOR(pc->Sel(operand++), StringInstr)
#define NULLARYTAGTESTS   INTINSTRVEC
#define NARYTAGTESTS      TRIPLE(pc->Sel(operand++), Int, IdDefs, Instr)
#define IDDEFSOPTINSTRVEC VECTOR(pc->Sel(operand++), IdDefsOptInstr)
#define NULLARYCONTESTS   VECTOR(pc->Sel(operand++), IdRefInstr)
#define NARYCONTESTS      VECTOR(pc->Sel(operand++), IdRefIdDefsInstr)
#define IDDEFSINSTRVEC    VECTOR(pc->Sel(operand++), IdDefsInstr)

class Disassembler {
private:
  static const u_int initialSize = 19; //--** to be determined

  std::FILE *file;
  Stack *todo;
  BlockHashTable *done;
  Queue *immediates;
  TagVal *pc;
  u_int operand;

  void Instr(word w) {
    TagVal *instr = TagVal::FromWordDirect(w);
    todo->SlowPush(instr->ToWord());
    fprintf(file, " %p", instr);
  }
  void LastInstr(word w) {
    TagVal *instr = TagVal::FromWordDirect(w);
    if (done->IsMember(w))
      fprintf(file, " %p", instr);
    else
      todo->SlowPush(instr->ToWord());
  }
  void Int(word w) {
    fprintf(file, " %d", Store::DirectWordToInt(w));
  }
  void Value(word value) {
    int i = Store::WordToInt(value);
    if (i != INVALID_INT)
      fprintf(file, " int(%d)", i);
    else {
      //--** treat chunks specially
      immediates->Enqueue(value);
      fprintf(file, " %p", value);
    }
  }
  void IdDef(word w) {
    TagVal *idDef = TagVal::FromWord(w);
    if (idDef == INVALID_POINTER)
      fprintf(file, " Wildcard");
    else
      fprintf(file, " IdDef(%d)", Store::DirectWordToInt(idDef->Sel(0)));
  }
  void IdDefs(word w) {
    VECTOR(w, IdDef);
  }
  void IdRef(word w) {
    TagVal *idRef = TagVal::FromWordDirect(w);
    switch (AbstractCode::GetIdRef(idRef)) {
    case AbstractCode::Immediate:
      fprintf(file, " Immediate(");
      Value(idRef->Sel(0));
      fprintf(file, " )");
      break;
    case AbstractCode::Local:
      fprintf(file, " Local(%d)", Store::DirectWordToInt(idRef->Sel(0)));
      break;
    case AbstractCode::LastUseLocal:
      fprintf(file, " LastUseLocal(%d)",
	      Store::DirectWordToInt(idRef->Sel(0)));
      break;
    case AbstractCode::Global:
      fprintf(file, " Global(%d)", Store::DirectWordToInt(idRef->Sel(0)));
      break;
    case AbstractCode::Toplevel:
      fprintf(file, " Toplevel(%d)", Store::DirectWordToInt(idRef->Sel(0)));
      break;
    }
  }
  void IdDefInstr(word w) {
    TUPLE(w, IdDef, Instr);
  }
  void IdDefArgs(word w) {
    ARGS(w, IdDef);
  }
  void IdDefArgsInstr(word w) {
    TUPLE(w, IdDefArgs, Instr);
  }
  void Label(word w) {
    fprintf(file, " %s",
	    UniqueString::FromWordDirect(w)->ToString()->ExportC());
  }
  void Template(word w) {
    TagVal *templ = TagVal::FromWordDirect(w);
    fprintf(file, " Template(");
    Int(templ->Sel(1));
    Int(templ->Sel(2));
    ARGS(templ->Sel(3), IdDef);
    Instr(templ->Sel(4));
    fprintf(file, " )");
  }
  void IntInstr(word w) {
    TUPLE(w, Int, Instr);
  }
  void RealInstr(word w) {
    TUPLE(w, Value, Instr);
  }
  void StringInstr(word w) {
    TUPLE(w, Value, Instr);
  }
  void IdDefsOpt(word w) {
    OPT(w, IdDefs);
  }
  void IdDefsOptInstr(word w) {
    TUPLE(w, IdDefsOpt, Instr);
  }
  void IdRefInstr(word w) {
    TUPLE(w, IdRef, Instr);
  }
  void IdRefIdDefsInstr(word w) {
    TRIPLE(w, IdRef, IdDefs, Instr);
  }
  void IdDefsInstr(word w) {
    TUPLE(w, IdDefs, Instr);
  }
public:
  Disassembler(std::FILE *f, TagVal *pc): file(f) {
    todo = Stack::New(initialSize);
    todo->SlowPush(pc->ToWord());
    done = BlockHashTable::New(initialSize);
    immediates = Queue::New(initialSize);
  }

  void Start();
  void DumpImmediates();
};

void Disassembler::Start() {
  while (!todo->IsEmpty()) {
    pc = TagVal::FromWordDirect(todo->Pop());
    if (done->IsMember(pc->ToWord()))
      continue;
    done->InsertItem(pc->ToWord(), Store::IntToWord(0));
    operand = 0;
    fprintf(file, "%p %s", pc, opcodes[AbstractCode::GetInstr(pc)]);
    switch (AbstractCode::GetInstr(pc)) {
    case AbstractCode::Kill:
      IDS LASTINSTR break;
    case AbstractCode::PutVar:
      ID IDREF LASTINSTR break;
    case AbstractCode::PutNew:
      ID STRING LASTINSTR break;
    case  AbstractCode::PutTag:
      ID INT IDREFS LASTINSTR break;
    case AbstractCode::PutCon:
      ID IDREF IDREFS LASTINSTR break;
    case AbstractCode::PutRef:
      ID IDREF LASTINSTR break;
    case AbstractCode::PutTup:
      ID IDREFS LASTINSTR break;
    case AbstractCode::PutPolyRec:
      ID LABELS IDREFS LASTINSTR break;
    case AbstractCode::PutVec:
      ID IDREFS LASTINSTR break;
    case AbstractCode::Close:
      ID IDREFS VALUE LASTINSTR break;
    case AbstractCode::Specialize:
      ID IDREFS TEMPLATE LASTINSTR break;
    case AbstractCode::AppPrim:
      VALUE IDREFS IDDEFINSTROPT break;
    case AbstractCode::AppVar:
      IDREF IDREFARGS IDDEFARGSINSTROPT break;
    case AbstractCode::DirectAppVar:
      IDREF IDREFARGS IDDEFARGSINSTROPT break;
    case AbstractCode::GetRef:
      ID IDREF LASTINSTR break;
    case AbstractCode::GetTup:
      IDDEFS IDREF LASTINSTR break;
    case AbstractCode::Sel:
      ID IDREF INT LASTINSTR break;
    case AbstractCode::LazyPolySel:
      IDS IDREF LABELS LASTINSTR break;
    case AbstractCode::Raise:
      IDREF break;
    case AbstractCode::Reraise:
      IDREF break;
    case AbstractCode::Try:
      INSTR IDDEF IDDEF INSTR break;
    case AbstractCode::EndTry:
      LASTINSTR break;
    case AbstractCode::EndHandle:
      LASTINSTR break;
    case AbstractCode::IntTest:
      IDREF INTINSTRVEC LASTINSTR break;
    case AbstractCode::CompactIntTest:
      IDREF INT INSTRS LASTINSTR break;
    case AbstractCode::RealTest:
      IDREF REALINSTRVEC LASTINSTR break;
    case AbstractCode::StringTest:
      IDREF STRINGINSTRVEC LASTINSTR break;
    case AbstractCode::TagTest:
      IDREF NULLARYTAGTESTS NARYTAGTESTS LASTINSTR break;
    case AbstractCode::CompactTagTest:
      IDREF IDDEFSOPTINSTRVEC LASTINSTR break;
    case AbstractCode::ConTest:
      IDREF NULLARYCONTESTS NARYCONTESTS LASTINSTR break;
    case AbstractCode::VecTest:
      IDREF IDDEFSINSTRVEC LASTINSTR break;
    case AbstractCode::Shared:
      STAMP LASTINSTR break;
    case AbstractCode::Return:
      IDREFARGS break;
    default:
      Error("AbstractCode::Disassemble: unknown instr tag");
    }
    fprintf(file, "\n");
  }
}

void Disassembler::DumpImmediates() {
  while (!immediates->IsEmpty()) {
    word value = immediates->Dequeue();
    fprintf(file, "\nValue at %p:\n\n", value);
    Debug::DumpTo(file, value);
  }
}

void AbstractCode::Disassemble(std::FILE *f, TagVal *pc) {
  Disassembler disassembler(f, pc);
  disassembler.Start();
  disassembler.DumpImmediates();
}
