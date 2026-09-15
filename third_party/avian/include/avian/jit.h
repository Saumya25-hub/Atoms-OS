/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_JIT_H
#define AVIAN_JIT_H

#include <avian/common.h>
#include <avian/classfile.h>
#include <stdint.h>
#include <stddef.h>

namespace avian {

enum JitStatus {
  JitSuccess = 0,
  JitUnsupportedOpcode = 1,
  JitOutOfMemory = 2,
  JitCompilationFailed = 3,
  JitFallbackToInterpreter = 4
};

typedef int64_t (*CompiledMethodFn)(int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3);

class CodeCache {
 public:
  CodeCache();
  ~CodeCache();

  bool initialize(size_t capacity = 64 * 1024);
  uint8_t* allocate(size_t bytes);
  bool commitExecutable(uint8_t* codePtr, size_t bytes);
  void clear();

  size_t totalCapacity() const { return capacity_; }
  size_t usedBytes() const { return offset_; }

 private:
  uint8_t* buffer_;
  size_t capacity_;
  size_t offset_;
  bool initialized_;
};

class JitCompiler {
 public:
  static const int HOT_INVOCATION_THRESHOLD = 5;

  JitCompiler();
  ~JitCompiler();

  bool initialize();
  JitStatus compileMethod(ClassFile* cls, MethodInfo* method, CompiledMethodFn* outFn);

  bool isEnabled() const { return enabled_; }
  void setEnabled(bool v) { enabled_ = v; }

  size_t compiledMethodCount() const { return compiledCount_; }
  CodeCache* codeCache() { return &codeCache_; }

 private:
  CodeCache codeCache_;
  bool enabled_;
  size_t compiledCount_;
};

} // namespace avian

#endif // AVIAN_JIT_H
