/*
 * Copyright 2012 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_CODEGEN_X64_JIT_COMPILER_X64_H_
#define V8_CODEGEN_X64_JIT_COMPILER_X64_H_

#include "third_party/v8/include/v8-platform.h"
#include "assembler-x64.h"

namespace v8 {
namespace internal {

class JitCompilerX64 {
public:
    static void* CompileSquareFunction(v8::PageAllocator* page_allocator);
    static void* CompileMultiplyFunction(v8::PageAllocator* page_allocator);
    static void FreeJitFunction(v8::PageAllocator* page_allocator, void* function_ptr);
};

} // namespace internal
} // namespace v8

#endif // V8_CODEGEN_X64_JIT_COMPILER_X64_H_
