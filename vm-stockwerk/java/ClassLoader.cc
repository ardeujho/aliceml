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
#pragma implementation "java/ClassLoader.hh"
#endif

#include <cstdio>
#include "adt/HashTable.hh"
#include "generic/Transients.hh"
#include "generic/ConcreteCode.hh"
#include "generic/Interpreter.hh"
#include "java/StackFrame.hh"
#include "java/ClassLoader.hh"
#include "java/ClassFile.hh"

class ClassTable: private HashTable {
private:
  static const u_int initialTableSize = 19; //--** to be determined
public:
  using Block::ToWord;

  static ClassTable *ClassTable::New() {
    return static_cast<ClassTable *>
      (HashTable::New(HashTable::BLOCK_KEY, initialTableSize));
  }
  static ClassTable *FromWordDirect(word x) {
    return static_cast<ClassTable *>(HashTable::FromWordDirect(x));
  }

  word Lookup(JavaString *name) {
    word wName = name->ToWord();
    if (IsMember(wName))
      return GetItem(wName);
    else
      return word(0);
  }
  void Insert(JavaString *name, word wClass) {
    word wName = name->ToWord();
    Assert(!IsMember(wName));
    InsertItem(wName, wClass);
  }
};

//
// BuildClassWorker
//

class BuildClassWorker: public Worker {
public:
  static BuildClassWorker *self;
private:
  BuildClassWorker() {}
public:
  static void Init() {
    self = new BuildClassWorker();
  }

  static void PushFrame(ClassInfo *classInfo);

  virtual Result Run();
  virtual const char *Identify();
  virtual void DumpFrame(word frame);
};

class BuildClassFrame: private StackFrame {
protected:
  enum { CLASS_INFO_POS, SIZE };
public:
  using Block::ToWord;

  static BuildClassFrame *New(ClassInfo *classInfo) {
    StackFrame *frame =
      StackFrame::New(BUILD_CLASS_FRAME, BuildClassWorker::self, SIZE);
    frame->InitArg(CLASS_INFO_POS, classInfo->ToWord());
    return static_cast<BuildClassFrame *>(frame);
  }
  static BuildClassFrame *FromWordDirect(word x) {
    StackFrame *frame = StackFrame::FromWordDirect(x);
    Assert(frame->GetLabel() == BUILD_CLASS_FRAME);
    return static_cast<BuildClassFrame *>(frame);
  }

  ClassInfo *GetClassInfo() {
    return ClassInfo::FromWordDirect(GetArg(CLASS_INFO_POS));
  }
};

void BuildClassWorker::PushFrame(ClassInfo *classInfo) {
  Scheduler::PushFrame(BuildClassFrame::New(classInfo)->ToWord());
}

Worker::Result BuildClassWorker::Run() {
  BuildClassFrame *frame =
    BuildClassFrame::FromWordDirect(Scheduler::GetFrame());
  ClassInfo *classInfo = frame->GetClassInfo();
  word wSuper = classInfo->GetSuper();
  if (Store::WordToTransient(wSuper) != INVALID_POINTER) {
    //--** detect ClassCircularityError
    Scheduler::currentData = wSuper;
    return Worker::REQUEST;
  }
  //--** if the class or interface named as the direct superclass of C is
  //--** in fact an interface, loading throws an IncompatibleClassChangeError
  Table *interfaces = classInfo->GetInterfaces();
  for (u_int i = interfaces->GetCount(); i--; ) {
    //--** detect ClassCircularityError
    word wInterface = interfaces->Get(i);
    if (Store::WordToTransient(wInterface) != INVALID_POINTER) {
      Scheduler::currentData = wInterface;
      return Worker::REQUEST;
    }
    //--** if any of the classes or interfaces named as direct
    //--** superinterfaces of C is not in fact an interface, loading
    //--** throws an IncompatibleClassChangeError
  }
  //--** if (!classInfo->Verify()) raise VerifyError
  Scheduler::PopFrame();
  Scheduler::nArgs = Scheduler::ONE_ARG;
  Scheduler::currentArgs[0] =
    classInfo->ToWord(); //--** return the class object
  return Worker::CONTINUE;
}

const char *BuildClassWorker::Identify() {
  return "BuildClassWorker";
}

void BuildClassWorker::DumpFrame(word frame) {
  BuildClassFrame *buildClassFrame = BuildClassFrame::FromWordDirect(frame);
  std::fprintf(stderr, "Build class %s\n",
	       buildClassFrame->GetClassInfo()->GetName()->ExportC());
}

//
// ResolveInterpreter
//

class ResolveInterpreter: public Interpreter {
public:
  enum type {
    RESOLVE_CLASS, RESOLVE_FIELD, RESOLVE_METHOD, RESOLVE_INTERFACE_METHOD
  };
  static ResolveInterpreter *self;
private:
  ResolveInterpreter() {}
public:
  static void Init() {
    self = new ResolveInterpreter();
  }

  virtual Result Run();
  virtual const char *Identify();
  virtual void DumpFrame(word frame);
  virtual void PushCall(Closure *closure);
};

class ResolveFrame: private StackFrame {
protected:
  enum {
    CLASS_LOADER_POS, RESOLVE_TYPE_POS, CLASS_POS, NAME_POS, DESCRIPTOR_POS,
    SIZE
  };
public:
  using Block::ToWord;

  static ResolveFrame *New(ClassLoader *classLoader, JavaString *name) {
    StackFrame *frame =
      StackFrame::New(RESOLVE_CLASS_FRAME, ResolveInterpreter::self, SIZE);
    frame->InitArg(CLASS_LOADER_POS, classLoader->ToWord());
    frame->InitArg(RESOLVE_TYPE_POS, ResolveInterpreter::RESOLVE_CLASS);
    frame->InitArg(NAME_POS, name->ToWord());
    return static_cast<ResolveFrame *>(frame);
  }
  static ResolveFrame *New(ClassLoader *classLoader,
			   ResolveInterpreter::type resolveType,
			   word theClass, JavaString *name,
			   JavaString *descriptor) {
    StackFrame *frame =
      StackFrame::New(RESOLVE_CLASS_FRAME, ResolveInterpreter::self, SIZE);
    frame->InitArg(CLASS_LOADER_POS, classLoader->ToWord());
    frame->InitArg(RESOLVE_TYPE_POS, resolveType);
    frame->InitArg(CLASS_POS, theClass);
    frame->InitArg(NAME_POS, name->ToWord());
    frame->InitArg(DESCRIPTOR_POS, descriptor->ToWord());
    return static_cast<ResolveFrame *>(frame);
  }
  static ResolveFrame *FromWordDirect(word x) {
    StackFrame *frame = StackFrame::FromWordDirect(x);
    Assert(frame->GetLabel() == RESOLVE_CLASS_FRAME);
    return static_cast<ResolveFrame *>(frame);
  }

  ClassLoader *GetClassLoader() {
    return ClassLoader::FromWordDirect(GetArg(CLASS_LOADER_POS));
  }
  ResolveInterpreter::type GetResolveType() {
    return static_cast<ResolveInterpreter::type>
      (Store::DirectWordToInt(GetArg(RESOLVE_TYPE_POS)));
  }
  word GetClass() {
    Assert(GetResolveType() != ResolveInterpreter::RESOLVE_CLASS);
    return GetArg(CLASS_POS);
  }
  JavaString *GetName() {
    return JavaString::FromWordDirect(GetArg(NAME_POS));
  }
  JavaString *GetDescriptor() {
    Assert(GetResolveType() != ResolveInterpreter::RESOLVE_CLASS);
    return JavaString::FromWordDirect(GetArg(DESCRIPTOR_POS));
  }
};

Worker::Result ResolveInterpreter::Run() {
  ResolveFrame *frame = ResolveFrame::FromWordDirect(Scheduler::GetFrame());
  switch (frame->GetResolveType()) {
  case RESOLVE_CLASS:
    {
      JavaString *name = frame->GetName();
      ClassFile *classFile = ClassFile::NewFromFile(name);
      ClassInfo *classInfo = classFile->Parse(frame->GetClassLoader());
      if (classInfo == INVALID_POINTER)
	; //--** raise ClassFormatError
      if (!classInfo->GetName()->Equals(name))
	; //--** raise NoClassDefFoundError
      Scheduler::PopFrame();
      BuildClassWorker::PushFrame(classInfo);
      return Worker::CONTINUE;
    }
  case RESOLVE_FIELD:
  case RESOLVE_METHOD:
  case RESOLVE_INTERFACE_METHOD:
    {
      word wClass = frame->GetClass();
      Class *theClass = Class::FromWord(wClass);
      if (theClass == INVALID_POINTER) {
	Scheduler::currentData = wClass;
	return Worker::REQUEST;
      }
    }
    break; //--**
  }
}

void ResolveInterpreter::PushCall(Closure *closure) {
  ClassLoader *classLoader = ClassLoader::FromWordDirect(closure->Sub(0));
  ResolveInterpreter::type resolveType =
    static_cast<ResolveInterpreter::type>
    (Store::DirectWordToInt(closure->Sub(1)));
  if (resolveType == ResolveInterpreter::RESOLVE_CLASS) {
    JavaString *name = JavaString::FromWordDirect(closure->Sub(2));
    Scheduler::PushFrame(ResolveFrame::New(classLoader, name)->ToWord());
  } else {
    word theClass = closure->Sub(2);
    JavaString *name = JavaString::FromWordDirect(closure->Sub(3));
    JavaString *descriptor = JavaString::FromWordDirect(closure->Sub(4));
    Scheduler::PushFrame(ResolveFrame::New(classLoader, resolveType, theClass,
					   name, descriptor)->ToWord());
  }
}

const char *ResolveInterpreter::Identify() {
  return "ResolveInterpreter";
}

void ResolveInterpreter::DumpFrame(word frame) {
  ResolveFrame *resolveClassFrame = ResolveFrame::FromWordDirect(frame);
  std::fprintf(stderr, "Resolve class %s\n",
	       resolveClassFrame->GetName()->ExportC());
}

//
// ClassLoader Method Implementations
//

void ClassLoader::Init() {
  BuildClassWorker::Init();
  ResolveInterpreter::Init();
}

ClassLoader *ClassLoader::New() {
  Block *b = Store::AllocBlock(JavaLabel::ClassLoader, SIZE);
  b->InitArg(CLASS_TABLE_POS, ClassTable::New()->ToWord());
  return static_cast<ClassLoader *>(b);
}

ClassTable *ClassLoader::GetClassTable() {
  return ClassTable::FromWordDirect(GetArg(CLASS_TABLE_POS));
}

word ClassLoader::ResolveClassByNeed(JavaString *name) {
  ConcreteCode *concreteCode = ConcreteCode::New(ResolveInterpreter::self, 0);
  Closure *closure = Closure::New(concreteCode->ToWord(), 3);
  closure->Init(0, ToWord());
  closure->Init(1, Store::IntToWord(ResolveInterpreter::RESOLVE_CLASS));
  closure->Init(2, name->ToWord());
  return Byneed::New(closure->ToWord())->ToWord();
}

word ClassLoader::ResolveType(JavaString *name) {
  ClassTable *classTable = GetClassTable();
  word wClass = classTable->Lookup(name);
  if (wClass == (word) 0) {
    u_int n = name->GetLength();
    u_wchar *p = name->GetBase();
    u_int dimensions = 0;
    while (dimensions < n && p[dimensions] == '[') dimensions++;
    p += dimensions;
    n -= dimensions;
    if (dimensions) { // array type
      Assert(n > 0);
      switch (n--, *p++) {
      case 'B':
	wClass = BaseArrayType::New(BaseType::Byte, dimensions)->ToWord();
	break;
      case 'C':
	wClass = BaseArrayType::New(BaseType::Char, dimensions)->ToWord();
	break;
      case 'D':
	wClass = BaseArrayType::New(BaseType::Double, dimensions)->ToWord();
	break;
      case 'F':
	wClass = BaseArrayType::New(BaseType::Float, dimensions)->ToWord();
	break;
      case 'I':
	wClass = BaseArrayType::New(BaseType::Int, dimensions)->ToWord();
	break;
      case 'J':
	wClass = BaseArrayType::New(BaseType::Long, dimensions)->ToWord();
	break;
      case 'S':
	wClass = BaseArrayType::New(BaseType::Short, dimensions)->ToWord();
	break;
      case 'Z':
	wClass = BaseArrayType::New(BaseType::Boolean, dimensions)->ToWord();
	break;
      case 'L':
	{
	  u_int i = 0;
	  while (p[i++] != ';');
	  n -= i + 1;
	  word classType = ResolveClassByNeed(JavaString::New(p, i));
	  wClass = ObjectArrayType::New(classType, dimensions)->ToWord();
	  break;
	}
      default:
	Error("invalid descriptor"); //--** return failed future?
      }
    } else {
      u_int i = 0;
      while (p[i++] != ';');
      n -= i + 1;
      wClass = ResolveClassByNeed(JavaString::New(p, i));
    }
    Assert(n == 0);
    classTable->Insert(name, wClass);
  }
  return wClass;
}

word ClassLoader::ResolveFieldRef(JavaString *className, JavaString *name,
				  JavaString *descriptor) {
  //--** also maintain a dictionary?
  ConcreteCode *concreteCode = ConcreteCode::New(ResolveInterpreter::self, 0);
  Closure *closure = Closure::New(concreteCode->ToWord(), 5);
  closure->Init(0, ToWord());
  closure->Init(1, Store::IntToWord(ResolveInterpreter::RESOLVE_FIELD));
  closure->Init(2, ResolveType(className));
  closure->Init(3, name->ToWord());
  closure->Init(4, descriptor->ToWord());
  return Byneed::New(closure->ToWord())->ToWord();
}

word ClassLoader::ResolveMethodRef(JavaString *className, JavaString *name,
				   JavaString *descriptor) {
  //--** also maintain a dictionary?
  ConcreteCode *concreteCode = ConcreteCode::New(ResolveInterpreter::self, 0);
  Closure *closure = Closure::New(concreteCode->ToWord(), 5);
  closure->Init(0, ToWord());
  closure->Init(1, Store::IntToWord(ResolveInterpreter::RESOLVE_METHOD));
  closure->Init(2, ResolveType(className));
  closure->Init(3, name->ToWord());
  closure->Init(4, descriptor->ToWord());
  return Byneed::New(closure->ToWord())->ToWord();
}

word ClassLoader::ResolveInterfaceMethodRef(JavaString *className,
					    JavaString *name,
					    JavaString *descriptor) {
  //--** also maintain a dictionary?
  ConcreteCode *concreteCode = ConcreteCode::New(ResolveInterpreter::self, 0);
  Closure *closure = Closure::New(concreteCode->ToWord(), 5);
  closure->Init(0, ToWord());
  u_int resolveType = ResolveInterpreter::RESOLVE_INTERFACE_METHOD;
  closure->Init(1, Store::IntToWord(resolveType));
  closure->Init(2, ResolveType(className));
  closure->Init(3, name->ToWord());
  closure->Init(4, descriptor->ToWord());
  return Byneed::New(closure->ToWord())->ToWord();
}
