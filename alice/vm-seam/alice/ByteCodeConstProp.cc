//
// Author:
//   Christian Mueller <cmueller@ps.uni-sb.de>
//
// Copyright:
//   Christian Mueller, 2005
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#if defined(INTERFACE)
#pragma implementation "alice/ByteCodeConstProp.hh"
#endif

#include <stack>
#include "alice/AbstractCode.hh"
#include "alice/AbstractCodeInterpreter.hh"
#include "alice/ByteCodeConstProp.hh"
#include "alice/Types.hh"
#include "alice/LazySelInterpreter.hh"


namespace {

  class ControlStack {
  private:
    std::stack<u_int> stack;
    
    u_int Pop(){
      u_int top = stack.top();
      stack.pop();
      return top;
    }
    
    void Push(u_int item){
      stack.push(item);
    }
    
  public:
    
    enum { VISIT, INLINE_EXIT, STOP };
    
    u_int PopCommand() {
      return Pop();
    }
    
    TagVal *PopInstr() {
      return reinterpret_cast<TagVal *>(Pop());
    }
      
    void PopInlineExit(u_int *localOffset, Vector **subst, Vector **inlineReturnIdDefs, IntMap **sharedInArity, InlineInfo **inlineInfo, AppVarInfo **appVarInfo, ConstPropInfo **constPropInfo) {
      *constPropInfo = reinterpret_cast<ConstPropInfo*>(Pop());
      *appVarInfo = reinterpret_cast<AppVarInfo*>(Pop());
      *inlineInfo = reinterpret_cast<InlineInfo*>(Pop());
      *sharedInArity = reinterpret_cast<IntMap*>(Pop());
      *inlineReturnIdDefs = reinterpret_cast<Vector*>(Pop());
      *subst = reinterpret_cast<Vector*>(Pop());
      *localOffset = Pop();
    }
    
    void PushInstr(TagVal *instr) {
      Push(reinterpret_cast<u_int>(instr));
      Push(VISIT);
    }
    
    void PushInstr(word instr) {
      PushInstr(TagVal::FromWordDirect(instr));
    }
    
    void PushInlineExit(u_int localOffset, Vector *subst, Vector *inlineReturnIdDefs, IntMap *sharedInArity, InlineInfo *inlineInfo, AppVarInfo *appVarInfo, ConstPropInfo *constPropInfo) {
      Push(localOffset);
      Push(reinterpret_cast<u_int>(subst));
      Push(reinterpret_cast<u_int>(inlineReturnIdDefs));
      Push(reinterpret_cast<u_int>(sharedInArity));
      Push(reinterpret_cast<u_int>(inlineInfo));
      Push(reinterpret_cast<u_int>(appVarInfo));
      Push(reinterpret_cast<u_int>(constPropInfo));
      Push(INLINE_EXIT);
    }
    
    void PushStop() {
      Push(STOP);
    }
    
    bool Empty() {
      return stack.empty();
    }
    
    u_int GetSize() {
      return stack.size();
    }
  };


  class ConstPropAnalyser {
  private:
    
    class Const {
    public:
      
      enum kind_t {
	KindNotYetAssigned, /** variable has not yet been assigned in the analysis */
	KindImmediate,      /** so far the variable appears to hold a constant (but possibly transient) value */
	KindTagVal,         /** so far the variable appears to hold a TagVal with constant tag, but we dont know any more than that. */
	KindUnknown         /** the variable has been assigned but we dont know more than that */
      };
      
      
      kind_t kind;
      word value;           /** immediate value or Store::IntToWord(tag) */
      
      
      Const(kind_t kind = KindNotYetAssigned, word value = INVALID_POINTER) : kind(kind), value(value) {}
      
      
      s_int ToTag() {
	
	if (kind == KindNotYetAssigned || kind == KindUnknown) {
	  return INVALID_INT;
	}
	
	if (kind == KindTagVal) {
	  return Store::DirectWordToInt(value);
	}
	
	Assert(kind == KindImmediate);
	
	if (PointerOp::IsInt(value)) {
	  return Store::DirectWordToInt(value);
	}
	
	Block *b = Store::WordToBlock(value);
	if (b != INVALID_POINTER) {
	  if (Alice::IsTag(b->GetLabel())) {
	    return reinterpret_cast<TagVal*>(b)->GetTag();
	  }
	  if (b->GetLabel() == Alice::BIG_TAG) {
	    return reinterpret_cast<BigTagVal*>(b)->GetTag();
	  }
	}
	
	return INVALID_INT;
      }
      
      
      word ToImmediate() {
	return kind == KindImmediate ? value : INVALID_POINTER;
      }
      
      
      s_int ToInt() {
	word imm = ToImmediate();
	return imm == INVALID_POINTER ? INVALID_INT : Store::WordToInt(imm);
      }
      
      
      static Const NotYetAssigned() {
	return Const(KindNotYetAssigned);
      }
      
      
      static Const Unknown() {
	return Const(KindUnknown);
      }
      
      
      static Const FromImmediate(word value) {
	value = LazySelInterpreter::Deref(value);
	return Const(KindImmediate, value);
      }
      
      
      static Const FromInt(s_int i) {
	return FromImmediate(Store::IntToWord(i));
      }
    
    
      static Const FromTag(u_int tag) {
	return Const(KindTagVal, Store::IntToWord(tag));
      }
    
    };
    
    
    Const *constants;
    Vector *subst;
    word concreteCode;
    TagVal *abstractCode;
    u_int localOffset;
    u_int inlineDepth;
    
    /**
    * IdDef Vector to bind with the result when the current inlined
    * function returns, or INVALID_POINTER
    */
    Vector *inlineReturnIdDefs;

    /**
     * AppVarInfo for AppVar instr that currently being inlined, or INVALID_POINTER when inlineDepth == 0 
     */
    AppVarInfo *inlineAppVar;

    InlineInfo *inlineInfo;
    ConstPropInfo *constPropInfo;

    
    u_int IdToId(word id) {
      return Store::DirectWordToInt(id) + localOffset;
    }
    
    
    Const IdRefToConst(word wIdRef) {
      TagVal *idRef = TagVal::FromWordDirect(wIdRef);

      switch(AbstractCode::GetIdRef(idRef)) {
	case AbstractCode::LastUseLocal:
	case AbstractCode::Local: {
	  return constants[IdToId(idRef->Sel(0))];
	}
	case AbstractCode::Global: {
	  u_int index = Store::DirectWordToInt(idRef->Sel(0));
	  TagVal *substIdRef = TagVal::FromWordDirect(subst->Sub(index));
	  
	  switch(AbstractCode::GetIdRef(substIdRef)) {
	  case AbstractCode::Immediate:
	    return Const::FromImmediate(substIdRef->Sel(0));
	  case AbstractCode::Local:
	    return constants[IdToId(substIdRef->Sel(0))];
	  default:
	    return Const::Unknown();
	  }
	}
	case AbstractCode::Immediate: {
	  return Const::FromImmediate(idRef->Sel(0));
	}
      }
    }
    
    
    s_int IdRefToTag(word idRef) {
      return IdRefToConst(idRef).ToTag();
    }
    
    
    word IdRefToImmediate(word idRef) {
      return IdRefToConst(idRef).ToImmediate();
    }
    
    
    s_int IdRefToInt(word idRef) {
      return IdRefToConst(idRef).ToInt();
    }
    
    
    bool AllIdRefsImmediate(Vector *idRefs) {
      for(u_int i=idRefs->GetLength(); i--; ) {
	if(IdRefToImmediate(idRefs->Sub(i)) == INVALID_POINTER) {
	  return false;
	}
      }
      return true;
    }
    
    
    void AssignConst(Const cons, u_int dest) {
      Const cur = constants[dest];
      
      if (cur.kind == Const::KindUnknown || cons.kind == Const::KindNotYetAssigned) {
	return;
      }
      if (cur.kind == Const::KindNotYetAssigned) {
	constants[dest] = cons;
	return;
      }
      if (cons.kind == Const::KindUnknown) {
	constants[dest] = Const::Unknown();
	return;
      }
      
      // one is KimdImmediate, one is KindTag
      if (cur.kind != cons.kind) {
	s_int tagCur = cur.ToTag();
	s_int tagCon = cons.ToTag();
	constants[dest] = (tagCur != INVALID_INT && tagCur == tagCon) ? Const::FromTag(tagCur) : Const::Unknown();
      }
      // they are both the same kind
      else {
	constants[dest] = (cur.value == cons.value) ? cur : Const::Unknown(); // TODO: use logical equality
      }
    }
    
    
    void AssignValue(word value, u_int dest) {
      AssignConst(Const::FromImmediate(value), dest);
    }
    
    
    void AssignFromLocal(u_int src, u_int dest) {
      AssignConst(constants[src], dest);
    }
    
    
    void AssignFromIdRef(word idRef, u_int dest) {
      AssignConst(IdRefToConst(idRef), dest);
    }
    
    
    void AssignIdDef(Const cons, word wIdDef) {
      TagVal *idDef = TagVal::FromWord(wIdDef);
      if (idDef != INVALID_POINTER) {
	AssignConst(cons, IdToId(idDef->Sel(0)));
      }
    }
    
    void AssignIdDefUnknown(word wIdDef) {
      AssignIdDef(Const::Unknown(), wIdDef);
    }
    
	
    void AssignIdDefsUnknown(Vector *idDefs) {
      for (u_int i=idDefs->GetLength(); i--; ) {
	AssignIdDefUnknown(idDefs->Sub(i));
      }
    }
    
    
    void AssignIdDefsFromTagVal(TagVal *values, Vector *idDefs) {
      for (u_int i=idDefs->GetLength(); i--; ) {
	AssignIdDef(Const::FromImmediate(values->Sel(i)), idDefs->Sub(i));
      }
    }
    
    
    void AssignIdsUnknown(Vector *ids) {
      for (u_int i=ids->GetLength(); i--; ) {
	AssignConst(Const::Unknown(), IdToId(ids->Sub(i)));
      }
    }


    Vector *ShiftIdDefs(Vector *srcs, s_int offset) {
      u_int size = srcs->GetLength();
      Vector *dsts = Vector::New(size);
      for(u_int i = size; i--; ) {
	TagVal *argOpt = TagVal::FromWord(srcs->Sub(i));
	if(argOpt != INVALID_POINTER) {
	  u_int id = Store::DirectWordToInt(argOpt->Sel(0)) + offset;
	  TagVal *newOpt = TagVal::New(Types::SOME, 1);
	  newOpt->Init(0, Store::IntToWord(id));
	  dsts->Init(i, newOpt->ToWord());
	} else {
	  dsts->Init(i, srcs->Sub(i));
	}
      }
      return dsts;
    }
    
    
    void NewTestInfo(TagVal *instr, word continuation, Vector *idDefs = INVALID_POINTER) {
      Tuple *info = Tuple::New(2);
      info->Init(0, (idDefs == INVALID_POINTER ? Vector::New(0) : idDefs)->ToWord());
      info->Init(1, continuation);
      constPropInfo->GetTestInfo()->Put(instr->ToWord(), info->ToWord());
    }
    
    
    void InlineCCC(Vector *srcIdRefs, Vector *destIdDefs) {
      if(destIdDefs == INVALID_POINTER)
	return;
      
      u_int nSrc = srcIdRefs->GetLength();
      u_int nDest = destIdDefs->GetLength();
      
      if(nSrc == nDest) {
	for(u_int i = nSrc; i--; ) {
	  TagVal *idDef = TagVal::FromWord(destIdDefs->Sub(i));
	  if(idDef != INVALID_POINTER) {
	    u_int dest = IdToId(idDef->Sel(0));
	    AssignFromIdRef(srcIdRefs->Sub(i), dest);
	  }
	}
      }
      else {
	AssignIdDefsUnknown(destIdDefs);
      }
    }

    
  public:
    ConstPropAnalyser(TagVal *abstractCode, word concreteCode, InlineInfo *inlineInfo) {
      this->abstractCode = abstractCode;
      this->concreteCode = concreteCode;
      this->inlineInfo = inlineInfo;
      localOffset = 0;
      inlineDepth = 0;
      inlineAppVar = INVALID_POINTER;
      subst = Vector::FromWordDirect(abstractCode->Sel(1));
      constants = new Const[inlineInfo->GetNLocals()];
      inlineReturnIdDefs = INVALID_POINTER;
      constPropInfo = ConstPropInfo::New();
    }
    ~ConstPropAnalyser() {
      delete[] constants;
    }
    void Run();
    ConstPropInfo *GetConstPropInfo() { return constPropInfo; }
  };


  void ConstPropAnalyser::Run() {
    
    IntMap *sharedInArity = AbstractCode::SharedInArity(abstractCode);
    ControlStack stack;
    
    stack.PushStop();
    AssignIdDefsUnknown(Vector::FromWordDirect(abstractCode->Sel(3)));
    stack.PushInstr(TagVal::FromWordDirect(abstractCode->Sel(5)));
    while (true) {
      switch(stack.PopCommand()) {
      case ControlStack::STOP: {
	return;
      }
      case ControlStack::INLINE_EXIT: {
	stack.PopInlineExit(&localOffset, &subst, &inlineReturnIdDefs, &sharedInArity, &inlineInfo, &inlineAppVar, &constPropInfo);
	inlineDepth--;
	break;
      }
      case ControlStack::VISIT: {
	TagVal *instr = stack.PopInstr();
	AbstractCode::instr instrOp = AbstractCode::GetInstr(instr);
	switch (instrOp) {
	  case AbstractCode::Raise:
	  case AbstractCode::Reraise:
	    break;
	  case AbstractCode::Return: {
	    // exit from an inlined callee
	    if(inlineDepth > 0) {
	      Vector *returnIdRefs = Vector::FromWordDirect(instr->Sel(0));
	      InlineCCC(returnIdRefs, inlineReturnIdDefs);
	    }
	    break;
	  }
	  case AbstractCode::Coord:
	  case AbstractCode::Entry:
	  case AbstractCode::Exit:
	  case AbstractCode::EndHandle:
	  case AbstractCode::EndTry:
	  case AbstractCode::Kill: {
	    u_int cp = AbstractCode::GetContinuationPos(instrOp);
	    stack.PushInstr(instr->Sel(cp)); 
	    break;
	  }
	  case AbstractCode::GetRef:
	  case AbstractCode::PutNew:
	  case AbstractCode::PutRef:
	  case AbstractCode::PutVec:
	  case AbstractCode::Specialize:
	  case AbstractCode::PutCon:
	  case AbstractCode::PutPolyRec:
	  case AbstractCode::PutTup:
	  case AbstractCode::Sel: {
	    AssignConst(Const::Unknown(), IdToId(instr->Sel(0)));
	    u_int cp = AbstractCode::GetContinuationPos(instrOp);
	    stack.PushInstr(instr->Sel(cp));
	    break;
	  }
	  case AbstractCode::Close: {
	    u_int dest =  IdToId(instr->Sel(0));
	    Vector *idRefs = Vector::FromWordDirect(instr->Sel(1));
	    
	    if (AllIdRefsImmediate(idRefs)) {
	      word parentCC = (inlineDepth == 0) ? concreteCode : inlineAppVar->GetClosure()->GetConcreteCode();
	      word wConcreteCode = AbstractCodeInterpreter::GetCloseConcreteCode(parentCC, instr);
	      TagVal *abstractCode = AbstractCodeInterpreter::ConcreteToAbstractCode(wConcreteCode);
	      Vector *subst = Vector::FromWordDirect(abstractCode->Sel(1));
	      u_int closureSize = AbstractCode::GetNumberOfGlobals(subst);
	      
	      Closure *cls = Closure::New(wConcreteCode, closureSize);
	      for (u_int i=0, j=0; i<idRefs->GetLength(); i++) {
		TagVal *idRef = TagVal::FromWordDirect(subst->Sub(i));
		if (AbstractCode::GetIdRef(idRef) == AbstractCode::Global) {
		  cls->Init(j++, IdRefToImmediate(idRefs->Sub(i)));
		}
	      }
	      constPropInfo->GetPutConstants()->Put(instr->ToWord(), cls->ToWord());
	      AssignValue(cls->ToWord(), dest);
	    }
	    else {
	      AssignConst(Const::Unknown(), dest);
	    }
	    
	    stack.PushInstr(instr->Sel(3));
	    break;
	  }
	  case AbstractCode::GetTup: {
	    AssignIdDefsUnknown(Vector::FromWordDirect(instr->Sel(0)));
	    stack.PushInstr(instr->Sel(2));
	    break;
	  }
	  case AbstractCode::LazyPolySel: {
	    AssignIdsUnknown(Vector::FromWordDirect(instr->Sel(0)));
	    stack.PushInstr(instr->Sel(3));
	    break;
	  }
	  case AbstractCode::PutVar: {
	    AssignFromIdRef(instr->Sel(1), IdToId(instr->Sel(0)));
	    stack.PushInstr(instr->Sel(2));
	    break;
	  }
	  case AbstractCode::PutTag: {
            u_int dest = IdToId(instr->Sel(0));
	    u_int maxTag = Store::DirectWordToInt(instr->Sel(1));
	    u_int tag = Store::DirectWordToInt(instr->Sel(2));
	    Vector *idRefs = Vector::FromWordDirect(instr->Sel(3));
	    
	    // TODO: track contents of Tags where only certain parts are constant
	    if (!Alice::IsBigTagVal(maxTag) && AllIdRefsImmediate(idRefs)) {
	      TagVal *tv = TagVal::New(tag, idRefs->GetLength());
	      for(u_int i=idRefs->GetLength(); i--; ) {
		tv->Init(i, IdRefToImmediate(idRefs->Sub(i)));
	      }
	      constPropInfo->GetPutConstants()->Put(instr->ToWord(), tv->ToWord());
	      AssignValue(tv->ToWord(), dest);
	    }
	    else {
	      AssignConst(Const::FromTag(tag), dest);
	    }
	    
	    stack.PushInstr(TagVal::FromWordDirect(instr->Sel(4))); 
	    break;
	  }
	  case AbstractCode::AppPrim: {
	    TagVal *contOpt = TagVal::FromWord(instr->Sel(2));
	    if(contOpt != INVALID_POINTER) {
	      Tuple *cont = Tuple::FromWordDirect(contOpt->Sel(0));
	      AssignIdDefUnknown(cont->Sel(0));
	      stack.PushInstr(cont->Sel(1));
	    }
	    else if (inlineReturnIdDefs != INVALID_POINTER) {
	      AssignIdDefsUnknown(inlineReturnIdDefs);
	    }
	    break;
	  }
	  case AbstractCode::AppVar: {
	    
	    // some AppVars are omitted
	    word wOmittedCont = inlineInfo->GetOmittedAppVars()->CondGet(instr->ToWord());
	    if (wOmittedCont != INVALID_POINTER) {
	      stack.PushInstr(wOmittedCont);
	    }
	    
	    // some AppVars are inlined
	    word wAppVarInfo = inlineInfo->GetInlineMap()->CondGet(instr->ToWord());
	    if (wAppVarInfo != INVALID_POINTER) {
	      
	      AppVarInfo *avi = AppVarInfo::FromWordDirect(wAppVarInfo);
	      TagVal *abstractCode = avi->GetAbstractCode();
	      
	      TagVal *contOpt = TagVal::FromWord(instr->Sel(3));
	      if(contOpt != INVALID_POINTER) {
		Tuple *cont = Tuple::FromWordDirect(contOpt->Sel(0));
		stack.PushInstr(cont->Sel(1));
	      }
	      
	    // String *name = AbstractCodeInterpreter::MakeProfileName(abstractCode);
//	      fprintf(stderr,"inline at depth %d: ???\n", inlineDepth);
  // 	    AbstractCode::Disassemble(stderr, TagVal::FromWordDirect(abstractCode->Sel(5)));
	      
	      stack.PushInlineExit(localOffset, subst, inlineReturnIdDefs, sharedInArity, inlineInfo, inlineAppVar, constPropInfo);
	      stack.PushInstr(TagVal::FromWordDirect(abstractCode->Sel(5)));
	      u_int offset = avi->GetLocalOffset();
	      
	      InlineCCC(avi->GetArgs(), ShiftIdDefs(Vector::FromWordDirect(abstractCode->Sel(3)), offset));
	      localOffset += offset;
	      subst = avi->GetSubst();
	      inlineDepth++;
	      inlineInfo = avi->GetInlineInfo();
	      inlineAppVar = avi;
	      Map *inlineMap = constPropInfo->GetInlineMap();
	      constPropInfo = ConstPropInfo::New();
	      inlineMap->Put(instr->ToWord(), constPropInfo->ToWord()); //TODO: could minimization make this kind of map unsound?
	      sharedInArity = AbstractCode::SharedInArity(abstractCode);
	      
	      if (contOpt != INVALID_POINTER) {
		Tuple *cont = Tuple::FromWordDirect(contOpt->Sel(0));
		inlineReturnIdDefs = ShiftIdDefs(Vector::FromWordDirect(cont->Sel(0)), -offset);
	      }
	      else if (inlineReturnIdDefs != INVALID_POINTER) {
		inlineReturnIdDefs = ShiftIdDefs(inlineReturnIdDefs, -offset);
	      }
	    }
	    // some AppVars are not omitted or inlined (but might be uncurried)
	    else {
	      TagVal *contOpt = TagVal::FromWord(instr->Sel(3));
	      if(contOpt != INVALID_POINTER) {
		Tuple *cont = Tuple::FromWordDirect(contOpt->Sel(0));
		AssignIdDefsUnknown(Vector::FromWordDirect(cont->Sel(0)));
		stack.PushInstr(cont->Sel(1));
	      }
	      else if (inlineReturnIdDefs != INVALID_POINTER) {
		AssignIdDefsUnknown(inlineReturnIdDefs);
	      }
	    }
	    break;
	  }
	  case AbstractCode::Try: {
	    stack.PushInstr(instr->Sel(0));
	    AssignIdDefUnknown(instr->Sel(1));
	    AssignIdDefUnknown(instr->Sel(2));
	    stack.PushInstr(instr->Sel(3));
	    break;
	  }
	  case AbstractCode::IntTest:
	  case AbstractCode::RealTest:
	  case AbstractCode::StringTest: {
	    Vector *tests = Vector::FromWordDirect(instr->Sel(1));
	    for (u_int i=tests->GetLength(); i--; ) {
	      Tuple *test = Tuple::FromWordDirect(tests->Sub(i));
	      stack.PushInstr(test->Sel(1));
	    }
	    stack.PushInstr(instr->Sel(2));
	    break;
	  }
	  case AbstractCode::VecTest: {
	    Vector *tests = Vector::FromWordDirect(instr->Sel(1));
	    for (u_int i=tests->GetLength(); i--; ) {
	      Tuple *test = Tuple::FromWordDirect(tests->Sub(i));
	      AssignIdDefsUnknown(Vector::FromWordDirect(test->Sel(0)));
	      stack.PushInstr(test->Sel(1));
	    }
	    stack.PushInstr(instr->Sel(2));
	    break;
	  }
	  case AbstractCode::CompactIntTest: {
	    s_int offset = Store::DirectWordToInt(instr->Sel(1));
	    Vector *tests = Vector::FromWordDirect(instr->Sel(2));
	    u_int nTests = tests->GetLength();
	    word els = instr->Sel(3);
	    
	    for (u_int i=nTests; i--; ) {
	      stack.PushInstr(tests->Sub(i));
	    }
	    stack.PushInstr(els);
	    
	    s_int i = IdRefToInt(instr->Sel(0));
	    if (i != INVALID_INT) {
	      i -= offset;
	      NewTestInfo(instr, i >= 0 && i < nTests ? tests->Sub(i) : els);
	    }
	    
	    break;
	  }
	  case AbstractCode::ConTest: {
	    Vector *tests0 = Vector::FromWordDirect(instr->Sel(1));
	    for(u_int i=tests0->GetLength(); i--; ) {
	      Tuple *test = Tuple::FromWordDirect(tests0->Sub(i));
	      stack.PushInstr(test->Sel(1));
	    }
	    Vector *testsN = Vector::FromWordDirect(instr->Sel(2));
	    for(u_int i=testsN->GetLength(); i--; ) {
	      Tuple *test = Tuple::FromWordDirect(testsN->Sub(i));
	      AssignIdDefsUnknown(Vector::FromWordDirect(test->Sel(1)));
	      stack.PushInstr(test->Sel(2));
	    }
	    stack.PushInstr(instr->Sel(3));
	    break;
	  }
	  case AbstractCode::TagTest: {
	    s_int tag = IdRefToTag(instr->Sel(0));
	    word value = IdRefToImmediate(instr->Sel(0));
	    Vector *tests0 = Vector::FromWordDirect(instr->Sel(2));
	    Vector *testsN = Vector::FromWordDirect(instr->Sel(3));
	    word els = instr->Sel(4);
	    
	    for (u_int i=tests0->GetLength(); i--; ) {
	      Tuple *test = Tuple::FromWordDirect(tests0->Sub(i));
	      stack.PushInstr(test->Sel(1));
	    }
	    for (u_int i=testsN->GetLength(); i--; ) {
	      Tuple *test = Tuple::FromWordDirect(testsN->Sub(i));
	      u_int testTag = Store::DirectWordToInt(test->Sel(0));
	      Vector *idDefs = Vector::FromWordDirect(test->Sel(1));
	      
	      if (tag == testTag && value != INVALID_POINTER) {
		AssignIdDefsFromTagVal(TagVal::FromWordDirect(value), idDefs);
	      }
	      else {
		AssignIdDefsUnknown(idDefs);
	      }
	      
	      stack.PushInstr(test->Sel(2));
	    }
	    stack.PushInstr(els);

	    // record statically determined branch
	    if (tag != INVALID_INT) {
	      word cont = INVALID_POINTER;
	      Vector *idDefs = INVALID_POINTER;
	      
	      for (u_int i=tests0->GetLength(); !cont && i--; ) {
		Tuple *test = Tuple::FromWordDirect(tests0->Sub(i));
		if (Store::DirectWordToInt(test->Sel(0)) == tag) {
		  cont = test->Sel(1);
		}
	      }
	      for (u_int i=testsN->GetLength(); !cont && i--; ) {
		Tuple *test = Tuple::FromWordDirect(testsN->Sub(i));
		if (Store::DirectWordToInt(test->Sel(0)) == tag) {
		  idDefs = Vector::FromWordDirect(test->Sel(1));
		  cont = test->Sel(2);
		}
	      }
	      if(!cont) {
		cont = els;
	      }
	      
	      NewTestInfo(instr, cont, idDefs);
	    }
	    
	    break;
	  }
	  case AbstractCode::CompactTagTest: {
	    s_int tag = IdRefToTag(instr->Sel(0));
	    word value = IdRefToImmediate(instr->Sel(0));
	    Vector *tests = Vector::FromWordDirect(instr->Sel(2));
	    TagVal *elseOpt = TagVal::FromWord(instr->Sel(3));
	    
	    for (u_int i=tests->GetLength(); i--; ) {
	      Tuple *test = Tuple::FromWordDirect(tests->Sub(i));
	      TagVal *idDefsOpt = TagVal::FromWord(test->Sel(0));
	      
	      if (idDefsOpt != INVALID_POINTER) {
	        Vector *idDefs = Vector::FromWordDirect(idDefsOpt->Sel(0));
		if (tag == i && value != INVALID_POINTER) {
		  AssignIdDefsFromTagVal(TagVal::FromWordDirect(value), idDefs);
		}
		else {
	          AssignIdDefsUnknown(idDefs);
		}
              }
              
              stack.PushInstr(test->Sel(1));
	    }
	    if (elseOpt != INVALID_POINTER) {
	      stack.PushInstr(elseOpt->Sel(0));
	    }
	    
	    // record statically determined branch
	    if (tag != INVALID_INT) {
	      if (tag < tests->GetLength()) {
		Tuple *test = Tuple::FromWordDirect(tests->Sub(tag));
		TagVal *idDefsOpt = TagVal::FromWord(test->Sel(0));
		Vector *idDefs = (idDefsOpt != INVALID_POINTER) ?
		  Vector::FromWordDirect(idDefsOpt->Sel(0)) : INVALID_POINTER;
		NewTestInfo(instr, test->Sel(1), idDefs);
	      }
	      else {
		NewTestInfo(instr, elseOpt->Sel(0));
	      }
	    }
	    
	    break;
	  }
	  case AbstractCode::Shared: {
	    word stamp = instr->Sel(0);
	    u_int inArity = Store::DirectWordToInt(sharedInArity->Get(stamp));
	    sharedInArity->Put(stamp, Store::IntToWord(--inArity));
	    if (inArity == 0) {
	      stack.PushInstr(instr->Sel(1));
	    }
	    break;
	  }
	  default: {
	    Assert(false);
	  }
	}
	break;
      }
      default: {
	Assert(false);
      }
      }
    }
  }

}

ConstPropInfo *ByteCodeConstProp::Analyse(TagVal *abstractCode, word concreteCode, InlineInfo *inlineInfo) {
  /*
   static u_int count = 0;
   Tuple *funCoord = Tuple::FromWordDirect(abstractCode->Sel(0));
   std::fprintf(stderr, "%"U_INTF". do constant propagation for %s (%p) at %s:%"S_INTF".%"S_INTF"\n",
 	       ++count,
 	       String::FromWordDirect(funCoord->Sel(1))->ExportC(),
 	       abstractCode,
 	       String::FromWordDirect(funCoord->Sel(0))->ExportC(),
 	       Store::DirectWordToInt(funCoord->Sel(2)),
 	       Store::DirectWordToInt(funCoord->Sel(3))); 
  AbstractCode::Disassemble(stderr, TagVal::FromWordDirect(abstractCode->Sel(5)));
  */
  ConstPropAnalyser mainPass(abstractCode, concreteCode,  inlineInfo);
  mainPass.Run();
  return mainPass.GetConstPropInfo();
}
