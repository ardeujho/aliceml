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
#pragma implementation "generic/IOHandler.hh"
#endif

#include <cstdlib>
#include <cstring>
#include <errno.h>

#if USE_WINSOCK
#include <winsock.h>
#define GetLastError() WSAGetLastError()
#elif USE_POSIX_SELECT
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#define GetLastError() errno
#elif USE_EPOLL
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#define GetLastError() errno
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#define GetLastError() errno
#endif

#include <sys/time.h>

#include "adt/Queue.hh"
#include "generic/RootSet.hh"
#include "generic/Transients.hh"
#include "generic/IOHandler.hh"

namespace {

  // Return error message string containing additional information based on
  // errno. The returned string is static and will be overwritten in a future
  // call. It is expected that the Error(...) function will be called soon
  // after this.
  char* ErrnoMessage(const char* msg) {
    static char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s (%s)", msg, strerror(errno));
    return buffer;
  }

  int max(int a, int b) {
    return a > b ? a : b;
  }
  
  const BlockLabel ENTRY_LABEL = static_cast<BlockLabel>(MIN_DATA_LABEL);

  class Entry: private Block {
  private:
    enum { FD_POS, FUTURE_POS, SIZE };
  public:
    using Block::ToWord;

    static Entry *New(int fd, Future *future) {
      Block *block = Store::AllocMutableBlock(ENTRY_LABEL, SIZE);
      block->InitArg(FD_POS, fd);
      block->InitArg(FUTURE_POS, future->ToWord());
      return static_cast<Entry *>(block);
    }
    static Entry *FromWordDirect(word w) {
      Block *block = Store::DirectWordToBlock(w);
      Assert(block->GetLabel() == ENTRY_LABEL);
      return static_cast<Entry *>(block);
    }

    int GetFD() {
      return static_cast<int>(Store::DirectWordToInt(GetArg(FD_POS)));
    }
    Future *GetFuture() {
      Transient *transient = Store::WordToTransient(GetArg(FUTURE_POS));
      Assert(transient != INVALID_POINTER &&
         transient->GetLabel() == FUTURE_LABEL);
      return static_cast<Future *>(transient);
    }
  };

  /**
   * A set of file-descriptors that are awaiting (read/write/connect)abilty
   */
  class Set: private Queue {
  private:
    static const u_int initialQueueSize = 8; //--** to be checked
  public:
    using Queue::ToWord;
    using Queue::Blank;

    static Set *New() {
      return static_cast<Set *>(Queue::New(initialQueueSize));
    }
    
    static Set *FromWordDirect(word w) {
      return static_cast<Set *>(Queue::FromWordDirect(w));
    }

    Future *Add(int fd) {
      for (u_int i=0; i<GetNumberOfElements(); i++) {
        Entry *entry = Entry::FromWordDirect(GetNthElement(i));
        if (entry->GetFD() == fd) {
          return entry->GetFuture();
        }
      }
      Entry *entry = Entry::New(fd, Future::New());
      Enqueue(entry->ToWord());
      return entry->GetFuture();
    }
    
    void Remove(int fd) {
      u_int n = GetNumberOfElements();
      for (u_int i = 0; i < n; i++) {
        Entry *entry = Entry::FromWordDirect(GetNthElement(i));
        if (entry->GetFD() == fd) {
          Future *future = entry->GetFuture();
          future->ScheduleWaitingThreads();
          future->Become(REF_LABEL, Store::IntToWord(0));
          Queue::RemoveNthElement(i);
          return;
        }
      }
    }

#if USE_EPOLL
    void EnterIntoFDSet(int epollFD) {
      for (u_int i = GetNumberOfElements(); i--; ) {
        int fd = Entry::FromWordDirect(GetNthElement(i))->GetFD();
        struct epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLOUT;
        int res = epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &event);
        if (res < 0) {
          Error(ErrnoMessage("Set::EnterIntoFDSet"));
        }
      }
    }
#else
    void EnterIntoFDSet(fd_set *fdSet, int *maxFD) {
      for (u_int i = GetNumberOfElements(); i--; ) {
        int fd = Entry::FromWordDirect(GetNthElement(i))->GetFD();
        *maxFD = max(*maxFD, fd);
        FD_SET(fd, fdSet);
      }
    }
#endif

#if USE_EPOLL
    void Schedule(int epollFD, struct epoll_event* events, int nevents) {
      for (int i = 0; i < nevents; ++i,++events) {
        for (u_int j = 0; j < GetNumberOfElements(); ++j) {
          Entry *entry = Entry::FromWordDirect(GetNthElement(j));
          if (entry->GetFD() == events->data.fd) {
            Future *future = entry->GetFuture();
            future->ScheduleWaitingThreads();
            future->Become(REF_LABEL, Store::IntToWord(0));
            Queue::RemoveNthElement(j);
            break;
          }
        }
      }
    }
#else
    void Schedule(fd_set *fdSet) {
      u_int n = GetNumberOfElements();
      for (u_int i = 0; i < n; i++) {
        again:
        Entry *entry = Entry::FromWordDirect(GetNthElement(i));
        if (FD_ISSET(entry->GetFD(), fdSet)) {
          Future *future = entry->GetFuture();
          future->ScheduleWaitingThreads();
          future->Become(REF_LABEL, Store::IntToWord(0));
          Queue::RemoveNthElement(i);
          if (i < --n) goto again;
        }
      }
    }
#endif
  };

#if USE_EPOLL
  word Pollable;
#else
  word Readable, Writable, Connected;
#endif
};

int IOHandler::SocketPair(int type, int *sv) {
#if !USE_WINSOCK
  return socketpair(PF_UNIX, type, 0, sv);
#else
  int newsock = socket(AF_INET, type, 0);
  if (newsock == INVALID_SOCKET) return -1;
  // bind the socket to any unused port
  struct sockaddr_in sock_in;
  sock_in.sin_family = AF_INET;
  sock_in.sin_port = 0;
  sock_in.sin_addr.s_addr = INADDR_ANY;
  if (bind(newsock, reinterpret_cast<struct sockaddr *>(&sock_in), sizeof(sock_in)) < 0)
    return -1;
  int len = sizeof(sock_in);
  if (getsockname(newsock, reinterpret_cast<struct sockaddr *>(&sock_in), &len) < 0) {
    closesocket(newsock);
    return -1;
  }
  listen(newsock, 2);
  // create a connecting socket
  int outsock = socket(AF_INET, type, 0);
  if (outsock < 0) {
    closesocket(newsock);
    return -1;
  }
  sock_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  // Do a connect and accept the connection
  if (connect(outsock, reinterpret_cast<struct sockaddr *>(&sock_in), sizeof(sock_in)) < 0) {
    closesocket(newsock);
    closesocket(outsock);
    return -1;
  }
  int insock = accept(newsock, reinterpret_cast<struct sockaddr *>(&sock_in), &len);
  if (insock < 0) {
    closesocket(newsock);
    closesocket(outsock);
    return -1;
  }
  closesocket(newsock);
  sv[0] = insock;
  sv[1] = outsock;
  return 0;
#endif
}

#if USE_WINSOCK
int IOHandler::defaultFD;
#endif


void IOHandler::Init() {
#if USE_WINSOCK
  WSADATA wsa_data;
  WORD req_version = MAKEWORD(1, 1);
  if (WSAStartup(req_version, &wsa_data) != 0)
    Error("no usable WinSock DLL found");

  int sv[2];
  if (SocketPair(SOCK_STREAM, sv) == -1)
    Error("socketpair failed");
  defaultFD = sv[0];
#endif
#if USE_EPOLL
  Pollable = Set::New()->ToWord();
  RootSet::Add(Pollable);
#else
  Readable = Set::New()->ToWord();
  Writable = Set::New()->ToWord();
  Connected = Set::New()->ToWord();
  RootSet::Add(Readable);
  RootSet::Add(Writable);
  RootSet::Add(Connected);
#endif
}

void IOHandler::Select(struct timeval *timeout) {
#if USE_EPOLL
  const int MAX_EVENTS = 64;
  int ms = timeout ? ((timeout->tv_sec * 1000) + (timeout->tv_usec / 1000)) : -1;
  struct epoll_event events[MAX_EVENTS];
  int epollFD = epoll_create(MAX_EVENTS);
  if (epollFD < 0) {
      Error(ErrnoMessage("IOHandler::Select"));
  }
  Set *PollableSet = Set::FromWordDirect(Pollable);
  PollableSet->EnterIntoFDSet(epollFD);

  int ret = epoll_wait(epollFD, events, MAX_EVENTS, ms);
  if (ret < 0) {
      close(epollFD);
      if (GetLastError() == EINTR)
        return;
      Error(ErrnoMessage("IOHandler::Select"));
    } else if (ret > 0) {
      PollableSet->Schedule(epollFD, events, ret);
    }
    close(epollFD);
#else
  Set *ReadableSet = Set::FromWordDirect(Readable);
  Set *WritableSet = Set::FromWordDirect(Writable);
  Set *ConnectedSet = Set::FromWordDirect(Connected);

  fd_set readFDs, writeFDs, exceptFDs;
  FD_ZERO(&readFDs);
  FD_ZERO(&writeFDs);
  FD_ZERO(&exceptFDs);
  int maxRead = -1, maxWrite = -1, maxExcept = -1;

  ReadableSet->EnterIntoFDSet(&readFDs, &maxRead);
  WritableSet->EnterIntoFDSet(&writeFDs, &maxWrite);
  ConnectedSet->EnterIntoFDSet(&writeFDs, &maxWrite);

#if USE_WINSOCK
  // winsock uses exceptFDs to notify of connection error
  ConnectedSet->EnterIntoFDSet(&exceptFDs, &maxExcept);
  // winsock select does not allow all wait sets to be
  // empty - therefore always wait on stdin
  FD_SET(defaultFD, &readFDs);
  maxRead = max(maxRead, defaultFD);
#endif

  int maxFD = max(max(maxRead, maxWrite), maxExcept);
  if (maxFD >= 0) {
    int ret = select(maxFD + 1,
                     maxRead == -1 ? NULL : &readFDs,
                     maxWrite == -1 ? NULL : &writeFDs,
                     maxExcept == -1 ? NULL : &exceptFDs,
                     timeout);
    if (ret < 0) {
      if (GetLastError() == EINTR)
        return;
      Error("IOHandler::Select");
    } else if (ret > 0) {
      ReadableSet->Schedule(&readFDs);
      WritableSet->Schedule(&writeFDs);
      ConnectedSet->Schedule(&writeFDs);
#if USE_WINSOCK
      ConnectedSet->Schedule(&exceptFDs);
#endif
    }
  }
#endif
}

void IOHandler::Poll() {
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  Select(&timeout);
}

void IOHandler::Block() {
  struct timeval *ptimeout = NULL;
#if USE_WINSOCK
  // signals (such as timer events) do not interrupt the select call,
  // therefore we must not block infinitely (so that signals are polled)
  //--** better solution - notify via a socket?
  struct timeval timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 100; // same as TIME_SLICE in SignalHandler.cc
  ptimeout = &timeout;
#endif
  Select(ptimeout);
}

void IOHandler::Purge() {
#if USE_EPOLL
  Set::FromWordDirect(Pollable)->Blank();
#else
  Set::FromWordDirect(Readable)->Blank();
  Set::FromWordDirect(Writable)->Blank();
  Set::FromWordDirect(Connected)->Blank();
#endif
}

bool IOHandler::IsReadable(int fd) {
#if USE_EPOLL
  struct epoll_event event;
  int epollFD = epoll_create(1);
  if (epollFD < 0) {
    Error(ErrnoMessage("IOHandler::IsReadable"));
  }
  event.data.fd = fd;
  event.events = EPOLLIN;
  if (epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &event) < 0) {
    Error(ErrnoMessage("IOHandler::IsReadable"));
  }

 retry:
  int ret = epoll_wait(epollFD, &event, 1, 0);
  if (ret < 0) {
      if (GetLastError() == EINTR)
        goto retry;
      close(epollFD);
      Error(ErrnoMessage("IOHandler::IsReadable"));
  }

  close(epollFD);
  return ret != 0;
#else
  fd_set readFDs;
  FD_ZERO(&readFDs);
  FD_SET(fd, &readFDs);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
 retry:
  int ret = select(fd + 1, &readFDs, NULL, NULL, &timeout);
  if (ret < 0) {
    if (GetLastError() == EINTR)
      goto retry;
    Error("IOHandler::IsReadable");
  }
  return ret != 0;
#endif
}

Future *IOHandler::WaitReadable(int fd) {
#if USE_EPOLL
  struct epoll_event event;
  int epollFD = epoll_create(1);
  if (epollFD < 0) {
    Error(ErrnoMessage("IOHandler::WaitReadable"));
  }
  event.data.fd = fd;
  event.events = EPOLLIN;
  if (epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &event) < 0) {
    Error(ErrnoMessage("IOHandler::WaitReadable"));
  }
 retry:
  int ret = epoll_wait(epollFD, &event, 1, 0);
  if (ret < 0) {
      if (GetLastError() == EINTR)
        goto retry;
      close(epollFD);
      Error(ErrnoMessage("IOHandler::WaitReadable"));
  } else if (ret == 0) {
    close(epollFD);
    return Set::FromWordDirect(Pollable)->Add(fd);
  }
  else {
    close(epollFD);
    return INVALID_POINTER;
  }
#else
  fd_set readFDs;
  FD_ZERO(&readFDs);
  FD_SET(fd, &readFDs);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
 retry:
  int ret = select(fd + 1, &readFDs, NULL, NULL, &timeout);
  if (ret < 0) {
    if (GetLastError() == EINTR)
      goto retry;
    Error("IOHandler::WaitReadable");
  } else if (ret == 0) {
    return Set::FromWordDirect(Readable)->Add(fd);
  } else {
    return INVALID_POINTER;
  }
#endif
}

bool IOHandler::IsWritable(int fd) {
#if USE_EPOLL
  struct epoll_event event;
  int epollFD = epoll_create(1);
  if (epollFD < 0) {
    Error(ErrnoMessage("IOHandler::IsWritable"));
  }
  event.data.fd = fd;
  event.events = EPOLLOUT;
  if (epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &event) < 0) {
    Error(ErrnoMessage("IOHandler::IsWritable"));
  }
 retry:
  int ret = epoll_wait(epollFD, &event, 1, 0);
  if (ret < 0) {
      if (GetLastError() == EINTR)
        goto retry;
      close(epollFD);
      Error(ErrnoMessage("IOHandler::IsWritable"));
  }

  close(epollFD);
  return ret != 0;
#else
  fd_set writeFDs;
  FD_ZERO(&writeFDs);
  FD_SET(fd, &writeFDs);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
 retry:
  int ret = select(fd + 1, &writeFDs, NULL, NULL, &timeout);
  if (ret < 0) {
    if (GetLastError() == EINTR)
      goto retry;
    Error("IOHandler::IsWritable");
  }
  return ret != 0;
#endif
}

Future *IOHandler::WaitWritable(int fd) {
#if USE_EPOLL
  struct epoll_event event;
  int epollFD = epoll_create(1);
  if (epollFD < 0) {
    Error(ErrnoMessage("IOHandler::WaitWritable"));
  }
  event.data.fd = fd;
  event.events = EPOLLOUT;
  if (epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &event) < 0) {
    Error(ErrnoMessage("IOHandler::WaitWritable"));
  }

 retry:
  int ret = epoll_wait(epollFD, &event, 1, 0);
  if (ret < 0) {
      if (GetLastError() == EINTR)
        goto retry;
      close(epollFD);
      Error(ErrnoMessage("IOHandler::WaitWritable"));
  } else if (ret == 0) {
    close(epollFD);
    return Set::FromWordDirect(Pollable)->Add(fd);
  }
  else {
    close(epollFD);
    return INVALID_POINTER;
  }
#else
  fd_set writeFDs;
  FD_ZERO(&writeFDs);
  FD_SET(fd, &writeFDs);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
 retry:
  int ret = select(fd + 1, NULL, &writeFDs, NULL, &timeout);
  if (ret < 0) {
    if (GetLastError() == EINTR)
      goto retry;
    Error("IOHandler::WaitWritable");
  } else if (ret == 0) {
    return Set::FromWordDirect(Writable)->Add(fd);
  } else {
    return INVALID_POINTER;
  }
#endif
}

Future *IOHandler::WaitConnected(int fd) {
#if USE_EPOLL
  return Set::FromWordDirect(Pollable)->Add(fd);
#else
  return Set::FromWordDirect(Connected)->Add(fd);
#endif
}

void IOHandler::Close(int fd) {
#if USE_EPOLL
  Set::FromWordDirect(Pollable)->Remove(fd);
#else
  Set::FromWordDirect(Readable)->Remove(fd);
  Set::FromWordDirect(Writable)->Remove(fd);
  Set::FromWordDirect(Connected)->Remove(fd);
#endif
}
