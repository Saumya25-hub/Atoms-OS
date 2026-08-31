# ATOMS OS RUNTIME COMPONENT & OWNERSHIP MAP

**Document ID:** ATRIX-PHASE7-MAP-001  
**Phase:** Phase 7 — Component Ownership & Architecture Map  
**Date:** 2026-08-26  

---

## 1. Subsystem Component Ownership Map

```text
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│ Component                  Implementation Source        Ownership Category   License             │
├──────────────────────────────────────────────────────────────────────────────────────────────────┤
│ Kernel Syscall Core        kernel/core/syscall/         ATOMS ORIGINAL       ATOMS Proprietary   │
│ VMM Page Mapper (mmap)     kernel/core/memory/vmm/      ATOMS ORIGINAL       ATOMS Proprietary   │
│ Futex Wait/Wake Queue      kernel/core/sync/            ATOMS ORIGINAL       ATOMS Proprietary   │
│ VFS Posix Bridge           kernel/vfs/                  ATOMS ORIGINAL       ATOMS Proprietary   │
│ User CRT0 (_start)         userspace/runtime/c/crt0.asm ATOMS INTEGRATION    MIT / ATOMS         │
│ C Standard Library Headers userspace/runtime/c/include/ ADAPTED (musl libc)  MIT License         │
│ Memory Allocator (malloc)  userspace/runtime/c/memory.c ADAPTED (dlmalloc)   Public Domain       │
│ Pthread Library (pthreads) userspace/runtime/c/pthread.cADAPTED (musl libc)  MIT License         │
│ Formatted stdio (printf)   userspace/runtime/c/stdio.c  ADAPTED (musl libc)  MIT License         │
│ C++ Operators (new/delete) userspace/runtime/cpp/       ADAPTED (LLVM libc++)Apache 2.0 + LLVM   │
│ C++ Standard Containers    userspace/runtime/cpp/       ADAPTED (LLVM libc++)Apache 2.0 + LLVM   │
│ C++ std::mutex & atomic    userspace/runtime/cpp/       ADAPTED (LLVM libc++)Apache 2.0 + LLVM   │
│ C++ std::chrono (time)     userspace/runtime/cpp/       ADAPTED (LLVM libc++)Apache 2.0 + LLVM   │
│ Phase 7 Verification Suite userspace/tests/runtime_test/ATOMS ORIGINAL       ATOMS Proprietary   │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
```
