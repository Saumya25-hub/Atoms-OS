/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_SYSTEM_SYSTEM_H
#define AVIAN_SYSTEM_SYSTEM_H

#include <avian/common.h>
#include <avian/util/allocator.h>
#include <avian/util/abort.h>
#include <avian/system/memory.h>

namespace avian {
namespace system {

class System : public avian::util::Aborter {
 public:
  typedef intptr_t Status;

  enum FileType {
    TypeUnknown,
    TypeDoesNotExist,
    TypeFile,
    TypeDirectory
  };

  class Thread {
   public:
    virtual ~Thread() {}
    virtual void interrupt() = 0;
    virtual bool getAndClearInterrupted() = 0;
    virtual void join() = 0;
    virtual void dispose() = 0;
  };

  class Runnable {
   public:
    virtual ~Runnable() {}
    virtual void attach(Thread* thread) = 0;
    virtual void run() = 0;
    virtual bool interrupted() = 0;
    virtual void setInterrupted(bool v) = 0;
  };

  class Mutex {
   public:
    virtual ~Mutex() {}
    virtual void acquire() = 0;
    virtual void release() = 0;
    virtual void dispose() = 0;
  };

  class Monitor {
   public:
    virtual ~Monitor() {}
    virtual bool tryAcquire(Thread* context) = 0;
    virtual void acquire(Thread* context) = 0;
    virtual void release(Thread* context) = 0;
    virtual void wait(Thread* context, int64_t time) = 0;
    virtual void notify(Thread* context) = 0;
    virtual void notifyAll(Thread* context) = 0;
    virtual void dispose() = 0;
  };

  class Region {
   public:
    virtual ~Region() {}
    virtual const uint8_t* start() const = 0;
    virtual size_t length() const = 0;
    virtual void dispose() = 0;
  };

  class Directory {
   public:
    virtual ~Directory() {}
    virtual const char* next() = 0;
    virtual void dispose() = 0;
  };

  virtual ~System() {}

  // Memory Allocation Subsystem
  virtual void* allocate(size_t size) = 0;
  virtual void* tryAllocate(size_t size) = 0;
  virtual void free(const void* p) = 0;
  virtual bool mprotect(void* p, size_t size, unsigned permissions) = 0;

  // Threading & Synchronization Subsystem
  virtual Thread* makeThread(Runnable* r) = 0;
  virtual Thread* currentThread() = 0;
  virtual Mutex* makeMutex() = 0;
  virtual Monitor* makeMonitor() = 0;

  // Timing & High-Resolution Clocks
  virtual int64_t now() = 0;
  virtual void sleep(int64_t ms) = 0;

  // File I/O & Storage Subsystem
  virtual int open(const char* path, int flags) = 0;
  virtual int read(int fd, void* buffer, size_t count) = 0;
  virtual int write(int fd, const void* buffer, size_t count) = 0;
  virtual void close(int fd) = 0;
  virtual Region* mmap(int fd, size_t offset, size_t length) = 0;
  virtual Directory* readDirectory(const char* path) = 0;
  virtual FileType stat(const char* path) = 0;

  // Diagnostic Console & Telemetry
  virtual void print(const char* message) = 0;
  virtual void printDec(int64_t val) = 0;
  virtual void printHex(uint64_t val) = 0;

  // Lifecycle
  virtual void abort() override = 0;
  virtual void exit(int code) = 0;
};

} // namespace system
} // namespace avian

namespace vm {
using namespace avian::system;
}

#endif // AVIAN_SYSTEM_SYSTEM_H
