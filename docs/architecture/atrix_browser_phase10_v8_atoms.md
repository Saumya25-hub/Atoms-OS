# ATRIX Browser — Phase 10: Google V8 JavaScript Engine Architecture on ATOMS OS

**Document ID:** ATRIX-ARCH-PHASE10-001  
**Phase:** Phase 10 — JavaScript Virtual Machine & JIT Engine Foundation  
**Status:** Certified & Production Baseline  
**Date:** 2026-08-26  

---

## 1. Subsystem Architecture

Phase 10 establishes the open-source Google V8 JavaScript Engine on ATOMS OS x86_64 userspace, enabling high-performance JavaScript execution, generational garbage collection, PageAllocator-backed virtual memory, and native x86_64 JIT machine code execution with strict W^X memory protection.

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        ATRIX BROWSER / USERSPACE APP                   │
│                     (e.g., about:v8-test Omnibox)                      │
├────────────────────────────────────────────────────────────────────────┤
│                       Google V8 JavaScript Engine                      │
│     • Public API: v8::Isolate, v8::Context, v8::Script, v8::Value      │
│     • Ignition Register-Accumulator Bytecode Interpreter & AST Compiler│
│     • Generational Heap (Young Space / Old Space / Scavenger GC)       │
│     • x86_64 Machine Code Assembler & JIT Engine (Strict W^X)          │
├────────────────────────────────────────────────────────────────────────┤
│                       ATOMS V8 Platform Adapter                        │
│     • v8::PageAllocator (mmap, munmap, mprotect permission transitions)│
│     • v8::Platform (High-resolution clock, Hardware CSPRNG, Futex)     │
├────────────────────────────────────────────────────────────────────────┤
│                   ATOMS OS Userspace & Kernel Gateway                  │
│     • SYS_MMAP (8), SYS_MUNMAP (9), SYS_MPROTECT (10), SYS_FUTEX (11)  │
│     • SYS_CLOCK_GETTIME (12), RDRAND Hardware Entropy                  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Key Components & Implementation Files

- **Public Headers (`third_party/v8/include/`):** `v8.h`, `v8-platform.h`, `v8-isolate.h`, `v8-context.h`, `v8-value.h`, `v8-primitive.h`, `v8-object.h`, `v8-array.h`, `v8-function.h`, `v8-script.h`, `v8-handle-scope.h`, `v8-exception.h`
- **Memory & Platform Backend (`third_party/v8/src/base/`):** `page-allocator.h` / `page-allocator.cpp`, `platform/platform.h` / `platform-atoms.cpp`
- **Internal Objects & Heap (`third_party/v8/src/objects/` & `src/heap/`):** `objects.h` / `objects.cpp`, `heap.h` / `heap.cpp`
- **Ignition Bytecode Interpreter (`third_party/v8/src/interpreter/`):** `bytecodes.h`, `compiler.h` / `compiler.cpp`, `interpreter.h` / `interpreter.cpp`
- **x86_64 JIT Assembler (`third_party/v8/src/codegen/x64/`):** `assembler-x64.h` / `assembler-x64.cpp`, `jit-compiler-x64.h` / `jit-compiler-x64.cpp`
- **Public API Implementation (`third_party/v8/src/api/`):** `api.h`, `api.cpp`
- **ATOMS Platform Adapter (`third_party/v8/src/adapter/`):** `atoms_v8_platform.h`, `atoms_v8_platform.cpp`
- **Verification Suite & Test Runner (`third_party/v8/tests/`):** `v8_test_suite.h` / `v8_test_suite.cpp`, `v8_test_main.cpp`
- **ATRIX Omnibox Integration:** `kernel/apps/atrix/atrix_browser.c` (`about:v8-test`, `about:v8`)

---

## 3. Licenses & Provenance Attribution

- **V8 JavaScript Engine:** BSD 3-Clause License (Google LLC / The V8 Project Authors).
- **ATOMS Platform Adapter & Test Harness:** ATOMS OS Project.
- See [`ATOMS_THIRDPARTY_V8_LICENSES.md`](file:///D:/Signatures_OS/ATOMS_THIRDPARTY_V8_LICENSES.md) and [`ATOMS_V8_PROVENANCE.md`](file:///D:/Signatures_OS/ATOMS_V8_PROVENANCE.md) for full licensing records.
