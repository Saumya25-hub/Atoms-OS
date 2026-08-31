/*
 * Copyright 2012 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "jit-compiler-x64.h"
#include "userspace/runtime/c/include/string.h"

namespace v8 {
namespace internal {

void* JitCompilerX64::CompileSquareFunction(v8::PageAllocator* page_allocator) {
    if (!page_allocator) return nullptr;

    // 1. Allocate Read-Write memory page (W^X initial state)
    size_t pageSize = page_allocator->AllocatePageSize();
    void* codeMem = page_allocator->AllocatePages(nullptr, pageSize, pageSize, v8::PageAllocator::kReadWrite);
    if (!codeMem) return nullptr;

    // 2. Assemble native AMD64 machine code
    AssemblerX64 masm;
    masm.PushRbp();       // 55
    masm.MovRbpRsp();     // 48 89 E5
    masm.MulsdXmm0Xmm0(); // F2 0F 59 C0 (xmm0 = xmm0 * xmm0)
    masm.PopRbp();        // 5D
    masm.Ret();           // C3

    memcpy(codeMem, masm.GetBuffer(), masm.GetSize());

    // 3. Memory fence / store serialization
    __asm__ volatile("" : : : "memory");

    // 4. Transition permission to Read-Execute (Strict W^X)
    bool ok = page_allocator->SetPermissions(codeMem, pageSize, v8::PageAllocator::kReadExecute);
    if (!ok) {
        page_allocator->FreePages(codeMem, pageSize);
        return nullptr;
    }

    return codeMem;
}

void* JitCompilerX64::CompileMultiplyFunction(v8::PageAllocator* page_allocator) {
    if (!page_allocator) return nullptr;

    size_t pageSize = page_allocator->AllocatePageSize();
    void* codeMem = page_allocator->AllocatePages(nullptr, pageSize, pageSize, v8::PageAllocator::kReadWrite);
    if (!codeMem) return nullptr;

    AssemblerX64 masm;
    masm.PushRbp();
    masm.MovRbpRsp();
    masm.MovRaxRdi();
    masm.ImulRaxRsi();
    masm.PopRbp();
    masm.Ret();

    memcpy(codeMem, masm.GetBuffer(), masm.GetSize());
    __asm__ volatile("" : : : "memory");

    bool ok = page_allocator->SetPermissions(codeMem, pageSize, v8::PageAllocator::kReadExecute);
    if (!ok) {
        page_allocator->FreePages(codeMem, pageSize);
        return nullptr;
    }

    return codeMem;
}

void JitCompilerX64::FreeJitFunction(v8::PageAllocator* page_allocator, void* function_ptr) {
    if (page_allocator && function_ptr) {
        page_allocator->FreePages(function_ptr, page_allocator->AllocatePageSize());
    }
}

} // namespace internal
} // namespace v8
