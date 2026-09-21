/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_MACHINE_H
#define AVIAN_MACHINE_H

#include <avian/common.h>
#include <avian/system/system.h>
#include <avian/heap/heap.h>
#include <avian/jni.h>
#include <avian/classfile.h>
#include <avian/zip.h>

namespace avian {

class Machine {
 public:
  enum State {
    StateUninitialized,
    StateBooting,
    StateRunning,
    StateShuttingDown,
    StateTerminated
  };

  virtual ~Machine() {}

  virtual bool boot() = 0;
  virtual bool shutdown() = 0;
  virtual State state() const = 0;

  virtual system::System* system() const = 0;
  virtual heap::Heap* heap() const = 0;

  virtual JavaVM* javaVM() = 0;
  virtual JNIEnv* jniEnv() = 0;

  virtual int executeClass(const char* classPath, int argc = 0, char** argv = nullptr) = 0;
  virtual int executeClassFromMemory(const uint8_t* data, size_t length, int argc = 0, char** argv = nullptr) = 0;
  virtual int executeJar(const char* jarPath, int argc = 0, char** argv = nullptr) = 0;
  virtual int executeJarFromMemory(const uint8_t* jarData, size_t jarLength, int argc = 0, char** argv = nullptr) = 0;

  virtual ClassFile* loadClass(const char* className) = 0;
  virtual void registerEmbeddedClass(const char* className, const uint8_t* data, size_t length) = 0;

  // Static field storage & retrieval
  virtual void setStaticField(const char* className, const char* fieldName, intptr_t value) = 0;
  virtual bool getStaticField(const char* className, const char* fieldName, intptr_t* outValue) = 0;

  // Standalone Resource Loading (BOFS / VFS / Embedded JAR)
  virtual bool getResource(const char* resourcePath, const uint8_t** outData, size_t* outSize, bool* outAllocated) = 0;
};

Machine* makeMachine(system::System* s, size_t heapInitialBytes, size_t heapMaxBytes);

} // namespace avian

namespace vm {
using namespace avian;
}

#endif // AVIAN_MACHINE_H
