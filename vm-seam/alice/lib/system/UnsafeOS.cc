//
// Authors:
//   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
//   Leif Kornstaedt <kornstaedt@ps.uni-sb.de>
//
// Copyright:
//   Thorsten Brunklaus, 2002
//   Leif Kornstaedt, 2002-2003
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#include <cstdio>
#include <cstdlib>
#include <cstring>


#if defined(__MINGW32__) || defined(_MSC_VER)
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#define Interruptible(res, call)		\
  int res;					\
  do {						\
    res = call;					\
  } while (res < 0 && errno == EINTR);
#define GetLastError() errno
#endif

#include "alice/NativeCodeJitter.hh"
#include "alice/Authoring.hh"

static word wBufferString;

// Also Needed for UnsafeUnix
word SysErrConstructor;
#include "SysErr.icc"

//
// UnsafeOS.FileSys Structure
//

DEFINE1(UnsafeOS_FileSys_openDir) {
#if defined(__MINGW32__) || defined(_MSC_VER)
  // dummy
  RETURN_UNIT;
#else
  DECLARE_STRING(name, x0);
  DIR *d = opendir(name->ExportC());
  if (!d) RAISE_SYS_ERR();

  RETURN(Store::UnmanagedPointerToWord(d));
#endif
} END

DEFINE1(UnsafeOS_FileSys_readDir) {
#if defined(__MINGW32__) || defined(_MSC_VER)
  // dummy
  RETURN_INT(0);
#else
  DECLARE_UNMANAGED_POINTER(d, x0);
  
  if (struct dirent *n = readdir((DIR *)d)) {
    TagVal *some = TagVal::New(1,1);
    some->Init(0, String::New(n->d_name)->ToWord());
    RETURN(some->ToWord());
  } else {
    RETURN_INT(0);
  }
#endif
} END

DEFINE1(UnsafeOS_FileSys_rewindDir) {
#if defined(__MINGW32__) || defined(_MSC_VER)
  RETURN_UNIT;
#else
  DECLARE_UNMANAGED_POINTER(d, x0);
  rewinddir((DIR *)d);

  RETURN_UNIT;
#endif
} END

DEFINE1(UnsafeOS_FileSys_closeDir) {
#if defined(__MINGW32__) || defined(_MSC_VER)
  // dummy
  RETURN_UNIT;
#else
  DECLARE_UNMANAGED_POINTER(d, x0);

  if (closedir((DIR *)d) != 0)
    RAISE_SYS_ERR();
  
  RETURN_UNIT;
#endif
} END

DEFINE1(UnsafeOS_FileSys_chDir) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  if (SetCurrentDirectory(name->ExportC()) == FALSE) RAISE_SYS_ERR();
#else
  Interruptible(res, chdir(name->ExportC()));
  if (res) RAISE_SYS_ERR();
#endif
  RETURN_UNIT;
} END

DEFINE0(UnsafeOS_FileSys_getDir) {
  String *buffer = String::FromWordDirect(wBufferString);
  u_int size = buffer->GetSize();
 retry:
#if defined(__MINGW32__) || defined(_MSC_VER)
  u_int n = GetCurrentDirectory(size, (CHAR *) buffer->GetValue());
  if (n == 0) RAISE_SYS_ERR();
  if (n > size) {
    size = n + 1;
    buffer = String::New(size);
    wBufferString = buffer->ToWord();
    goto retry;
  }
#else
  if (getcwd((char *) buffer->GetValue(), size) == NULL) {
    if (errno != ERANGE) RAISE_SYS_ERR();
    size = size * 3 / 2;
    buffer = String::New(size);
    wBufferString = buffer->ToWord();
    goto retry;
  }
#endif
  RETURN(String::New((char *) buffer->GetValue())->ToWord());
} END

DEFINE1(UnsafeOS_FileSys_mkDir) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  if (CreateDirectory(name->ExportC(), NULL) == FALSE) RAISE_SYS_ERR();
#else
  int res = mkdir(name->ExportC(), S_IRWXU | S_IRWXG | S_IRWXO);
  if (res) RAISE_SYS_ERR();
#endif
  RETURN_UNIT;
} END

DEFINE1(UnsafeOS_FileSys_rmDir) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  if (RemoveDirectory(name->ExportC()) == FALSE) RAISE_SYS_ERR();
#else
  if (rmdir(name->ExportC())) RAISE_SYS_ERR();
#endif
  RETURN_UNIT;
} END

DEFINE1(UnsafeOS_FileSys_isDir) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  DWORD attr = GetFileAttributes(name->ExportC());
  if (attr == INVALID_FILE_ATTRIBUTES) RAISE_SYS_ERR();
  RETURN_BOOL(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat info;
  Interruptible(res, stat(name->ExportC(), &info));
  if (res) RAISE_SYS_ERR();
  RETURN_BOOL(S_ISDIR(info.st_mode));
#endif
} END

DEFINE1(UnsafeOS_FileSys_isLink) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  DWORD attr = GetFileAttributes(name->ExportC());
  if (attr == INVALID_FILE_ATTRIBUTES) RAISE_SYS_ERR();
  RETURN_BOOL(false); // no support for symbolic links
#else
  struct stat info;
  Interruptible(res, lstat(name->ExportC(), &info));
  if (res) RAISE_SYS_ERR();
  RETURN_BOOL(S_ISLNK(info.st_mode));
#endif
} END

DEFINE1(UnsafeOS_FileSys_readLink) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  // raise SysErr unconditionally
  name = name;
  ConVal *sysErr =
    ConVal::New(Store::DirectWordToBlock(SysErrConstructor), 2);
  sysErr->Init(0, String::New("symbolic links not supported")->ToWord());
  sysErr->Init(1, Store::IntToWord(Types::NONE));
  RAISE(sysErr->ToWord());
#else
  String *buffer = String::FromWordDirect(wBufferString);
  u_int size = buffer->GetSize();
 retry:
  int res = readlink(name->ExportC(), (char *) buffer->GetValue(), size);
  if (res < 0) RAISE_SYS_ERR();
  if (STATIC_CAST(u_int, res) == size) {
    size = size * 3 / 2;
    buffer = String::New(size);
    wBufferString = buffer->ToWord();
    goto retry;
  }
  RETURN(String::New((char *) buffer->GetValue(), res)->ToWord());
#endif
} END

DEFINE1(UnsafeOS_FileSys_fileSize) {
  DECLARE_STRING(name, x0);
  //--** truncates the file size if not representable
#if defined(__MINGW32__) || defined(_MSC_VER)
  HANDLE hFile =
    CreateFile(name->ExportC(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
	       FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) RAISE_SYS_ERR();
  DWORD n = GetFileSize(hFile, NULL);
  if (n == INVALID_FILE_SIZE) RAISE_SYS_ERR();
  CloseHandle(hFile);
  RETURN_INT(n);
#else
  struct stat info;
  Interruptible(res, stat(name->ExportC(), &info));
  if (res) RAISE_SYS_ERR();
  RETURN_INT(info.st_size);
#endif
} END

DEFINE1(UnsafeOS_FileSys_modTime) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  HANDLE hFile =
    CreateFile(name->ExportC(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
	       FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) RAISE_SYS_ERR();
  FILETIME fileTime;
  if (GetFileTime(hFile, NULL, NULL, &fileTime) == FALSE) RAISE_SYS_ERR();
  CloseHandle(hFile);
  BigInt *b = BigInt::New((unsigned int)fileTime.dwHighDateTime);
  mpz_mul_2exp(b->big(), b->big(), 32);
  mpz_add_ui(b->big(), b->big(), fileTime.dwLowDateTime);
  mpz_fdiv_q_ui(b->big(), b->big(), 10000);
  RETURN_INTINF(b);
#else
  struct stat info;
  Interruptible(res, stat(name->ExportC(), &info));
  if (res) RAISE_SYS_ERR();
  BigInt *b = BigInt::New((unsigned int)info.st_mtime);
  mpz_mul_ui(b->big(), b->big(), 1000);
  RETURN_INTINF(b);
#endif
} END

DEFINE1(UnsafeOS_FileSys_remove) {
  DECLARE_STRING(name, x0);
#if defined(__MINGW32__) || defined(_MSC_VER)
  if (DeleteFile(name->ExportC()) == FALSE) RAISE_SYS_ERR();
#else
  Interruptible(res, unlink(name->ExportC()));
  if (res) RAISE_SYS_ERR();
#endif
  RETURN_UNIT;
} END

DEFINE0(UnsafeOS_FileSys_tmpName) {
#if defined(__MINGW32__) || defined(_MSC_VER)
  String *buffer = String::FromWordDirect(wBufferString);
  u_int size = buffer->GetSize();
 retry:
  DWORD res = GetTempPath(size, (char *) buffer->GetValue());
  if (res == 0) RAISE_SYS_ERR();
  if (res > size) {
    size = size * 3 / 2;
    buffer = String::New(size);
    wBufferString = buffer->ToWord();
    goto retry;
  }
  String *name = String::New(res + 10);
  char *s = (char *) name->GetValue();
  static int counter = 0;
  while (true) {
    std::sprintf(s, "%salice%d", buffer->GetValue(), counter);
    counter = (counter++) % 10000;
    if (GetFileAttributes(s) == INVALID_FILE_ATTRIBUTES)
      break;
  }
  RETURN(name->ToWord());
#else
  static const char path[] = "/tmp/aliceXXXXXX";
  String *s = String::New(path, sizeof(path));
  mkstemp(reinterpret_cast<char *>(s->GetValue()));
  RETURN(s->ToWord());
#endif
} END

DEFINE0(UnsafeOS_FileSys_getHomeDir) {
  String *buffer = String::FromWordDirect(wBufferString);
  u_int size = buffer->GetSize();
#if defined(__MINGW32__) || defined(_MSC_VER)
  ITEMIDLIST* pidl;
  HRESULT hRes = SHGetSpecialFolderLocation( NULL, CSIDL_PERSONAL, &pidl );
  if (hRes==NOERROR) {
    SHGetPathFromIDList( pidl, (CHAR *) buffer->GetValue());
  } else {
    RAISE_SYS_ERR();
  }

  IMalloc* palloc; 
  hRes = SHGetMalloc(&palloc); 
  palloc->Free( (void*)pidl ); 
  palloc->Release();
  RETURN(String::New((char *) buffer->GetValue())->ToWord());
#else
  char *envVal = std::getenv("HOME");

  if (envVal==NULL) {
  retry:
    if (getcwd((char *) buffer->GetValue(), size) == NULL) {
      if (errno != ERANGE) RAISE_SYS_ERR();
      size = size * 3 / 2;
      buffer = String::New(size);
      wBufferString = buffer->ToWord();
      goto retry;
    }
    RETURN(String::New((char *) buffer->GetValue())->ToWord());
  }
  RETURN(String::New(envVal)->ToWord());
#endif
} END

DEFINE0(UnsafeOS_FileSys_getApplicationConfigDir) {
  String *buffer = String::FromWordDirect(wBufferString);
  u_int size = buffer->GetSize();
#if defined(__MINGW32__) || defined(_MSC_VER)
  ITEMIDLIST* pidl;
  HRESULT hRes = SHGetSpecialFolderLocation( NULL, CSIDL_APPDATA, &pidl );
  if (hRes==NOERROR) {
    SHGetPathFromIDList( pidl, (CHAR *) buffer->GetValue());
  } else {
    RAISE_SYS_ERR();
  }

  IMalloc* palloc; 
  hRes = SHGetMalloc(&palloc); 
  palloc->Free( (void*)pidl ); 
  palloc->Release();
  RETURN(String::New((char *) buffer->GetValue())->ToWord());
#else
  char *envVal = std::getenv("HOME");

  if (envVal==NULL) {
  retry:
    if (getcwd((char *) buffer->GetValue(), size) == NULL) {
      if (errno != ERANGE) RAISE_SYS_ERR();
      size = size * 3 / 2;
      buffer = String::New(size);
      wBufferString = buffer->ToWord();
      goto retry;
    }
    strcat((char *) buffer->GetValue(), "/.");
    RETURN(String::New((char *) buffer->GetValue())->ToWord());
  }
  strcpy((char *) buffer->GetValue(), envVal);
  strcat((char *) buffer->GetValue(), "/.");
  RETURN(String::New((char *) buffer->GetValue())->ToWord());
#endif
} END

static word UnsafeOS_FileSys() {
  Record *record = Record::New(17);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "chDir",
		 UnsafeOS_FileSys_chDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "getDir",
		 UnsafeOS_FileSys_getDir, 0);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "mkDir",
		 UnsafeOS_FileSys_mkDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "rmDir",
		 UnsafeOS_FileSys_rmDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "isDir",
		 UnsafeOS_FileSys_isDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "isLink",
		 UnsafeOS_FileSys_isLink, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "readLink",
		 UnsafeOS_FileSys_readLink, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "fileSize",
		 UnsafeOS_FileSys_fileSize, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "modTime",
		 UnsafeOS_FileSys_modTime, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "remove",
		 UnsafeOS_FileSys_remove, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "tmpName",
		 UnsafeOS_FileSys_tmpName, 1);
  INIT_STRUCTURE(record, "UnsafeOS_FileSys", "openDir",
		 UnsafeOS_FileSys_openDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS_FileSys", "readDir",
		 UnsafeOS_FileSys_readDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS_FileSys", "rewindDir",
		 UnsafeOS_FileSys_rewindDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS_FileSys", "closeDir",
		 UnsafeOS_FileSys_closeDir, 1);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "getHomeDir",
		 UnsafeOS_FileSys_getHomeDir, 0);
  INIT_STRUCTURE(record, "UnsafeOS.FileSys", "getApplicationConfigDir",
		 UnsafeOS_FileSys_getApplicationConfigDir, 0);
  return record->ToWord();
}

//
// UnsafeOS.Process Structure
//

DEFINE1(UnsafeOS_Process_system) {
  //--** Windows: NT/2000? see Mozart implementation
  //--** Unix: interruptibility? see Mozart implementation
  DECLARE_STRING(command, x0);
  RETURN_INT(system(command->ExportC()));
} END

DEFINE1(UnsafeOS_Process_terminate) {
  DECLARE_INT(code, x0);
#if PROFILE
  Profiler::DumpInfo();
#endif
#if defined(JIT_APPLY_STATISTIC)
  extern void DumpApplyStatistics();
  DumpApplyStatistics();
#endif
#if HAVE_LIGHTNING && defined(INSTRUCTION_COUNTS)
  NativeCodeJitter::DumpInstructionCounts();
#endif
  EXIT(code);
} END

DEFINE1(UnsafeOS_Process_atExn) {
  DECLARE_CLOSURE(closure, x0);
  TaskStack::AddExnClosure(closure->ToWord());
  RETURN_UNIT;
} END

#if defined(__MINGW32__) || defined(_MSC_VER)
// Platform SDK: 32767 maximum size excluding zero
#define MAX_ENV_VALUE_SIZE 32768
static char envValBuf[MAX_ENV_VALUE_SIZE];
#endif

DEFINE1(UnsafeOS_Process_getEnv) {
  DECLARE_STRING(envVar, x0);
  char *envVal;
#if defined(__MINGW32__) || defined(_MSC_VER)
  if (!GetEnvironmentVariable(envVar->ExportC(), envValBuf, MAX_ENV_VALUE_SIZE))
    envVal = NULL;
  else
    envVal = envValBuf;
#else
  envVal = std::getenv(envVar->ExportC());
#endif
  if (envVal != NULL) {
    TagVal *val = TagVal::New(Types::SOME, 1);
    val->Init(0, String::New(envVal)->ToWord());
    RETURN(val->ToWord());
  } else
    RETURN(Store::IntToWord(Types::NONE));
} END

static word UnsafeOS_Process() {
  Record *record = Record::New(6);
  record->Init("success", Store::IntToWord(0));
  record->Init("failure", Store::IntToWord(1));
  INIT_STRUCTURE(record, "UnsafeOS.Process", "system",
		 UnsafeOS_Process_system, 1);
  INIT_STRUCTURE(record, "UnsafeOS.Process", "terminate",
		 UnsafeOS_Process_terminate, 1);
  INIT_STRUCTURE(record, "UnsafeOS.Process", "atExn",
		 UnsafeOS_Process_atExn, 1);
  INIT_STRUCTURE(record, "UnsafeOS.Process", "getEnv",
		 UnsafeOS_Process_getEnv, 1);
  return record->ToWord();
}

//
// UnsafeOS Structure
//

DEFINE2(UnsafeOS_SysErr) {
  Block *ccVal = Store::DirectWordToBlock(SysErrConstructor);
  ConVal *conVal = ConVal::New(ccVal, 2);
  conVal->Init(0, x0);
  conVal->Init(1, x1);
  RETURN(conVal->ToWord());
} END

DEFINE1(UnsafeOS_errorMsg) {
  DECLARE_INT(errorCode, x0);
  RETURN(ErrorCodeToString(errorCode)->ToWord());
} END

AliceDll word UnsafeOS() {
  SysErrConstructor =
    UniqueConstructor::New("SysErr", "OS.SysErr")->ToWord();
  RootSet::Add(SysErrConstructor);
  wBufferString = String::New(1024)->ToWord();
  RootSet::Add(wBufferString);

  Record *record = Record::New(5);
  record->Init("'SysErr", SysErrConstructor);
  INIT_STRUCTURE(record, "UnsafeOS", "SysErr",
		 UnsafeOS_SysErr, 2);
  INIT_STRUCTURE(record, "UnsafeOS", "errorMsg",
		 UnsafeOS_errorMsg, 1);
  record->Init("FileSys$", UnsafeOS_FileSys());
  record->Init("Process$", UnsafeOS_Process());
  RETURN_STRUCTURE("UnsafeOS$", record);
}
