# PHASE 10 PATCH PLAN: V8 x86_64 JAVASCRIPT ENGINE BRING-UP

**Document ID:** ATRIX-PHASE10-PLAN-001  
**Phase:** TASK 3 — ARCHITECT TEAM  
**Target Subsystem:** Google V8 JavaScript Engine, Memory Allocator (mmap/W^X), Ignition Bytecode Interpreter, Scavenger/Mark-Sweep Heap & x86_64 JIT Code Generator  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Architectural Scope & Target Pipeline

Phase 10 integrates the real Google V8 JavaScript engine architecture into ATOMS OS, providing full JS execution, generational garbage collection, PageAllocator-backed virtual memory, and native x86_64 JIT code generation:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        ATRIX BROWSER / USERSPACE APP                   │
│                     (e.g., about:v8-test Omnibox)                      │
├────────────────────────────────────────────────────────────────────────┤
│                       Google V8 JavaScript Engine                      │
│     • Public API: v8::Isolate, v8::Context, v8::Script, v8::Value      │
│     • Ignition Bytecode Interpreter & AST Compiler                     │
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

## 2. Target Platform Identity & Configuration Decisions

| Component / Layer | Architectural Decision | Technical Rationale |
|:---|:---|:---|
| **V8 Memory Management** | `v8::PageAllocator` over `SYS_MMAP` | Direct virtual page allocation with 4KB alignment. |
| **W^X Security Model** | `mprotect(PROT_READ \| PROT_EXEC)` | JIT emits machine code in RW memory, then enforces RX before execution. |
| **Interpreter Architecture** | Register-based accumulator VM | Matches Ignition design: compact bytecodes with fast dispatch. |
| **Garbage Collector** | Generational Scavenger + Mark-Sweep | High-throughput copying GC for young objects; mark-sweep for old heap. |
| **Native JIT Backend** | Direct x86_64 Machine Code Assembler | Emits native AMD64 opcodes (`0x55` `push rbp`, `0x48 0x89 0xE5` `mov rbp, rsp`, etc.). |
| **Synchronization** | `SYS_FUTEX` (11) & C++20 `<atomic>` | Zero-overhead userspace locks with kernel sleep on contention. |
| **Clock Source** | `SYS_CLOCK_GETTIME` (12) | Nanosecond monotonic timing for performance telemetry. |

---

## 3. Files to Create and Modify

### 3.1 V8 Public Headers (`third_party/v8/include/`)
- `v8.h`, `v8-platform.h`, `v8-isolate.h`, `v8-context.h`, `v8-value.h`, `v8-primitive.h`, `v8-object.h`, `v8-array.h`, `v8-function.h`, `v8-script.h`, `v8-handle-scope.h`, `v8-exception.h`

### 3.2 V8 Base & Page Allocator (`third_party/v8/src/base/`)
- `page-allocator.h` / `page-allocator.cpp`, `platform/platform.h` / `platform/platform-atoms.cpp`

### 3.3 V8 Generational Heap & Objects (`third_party/v8/src/heap/` & `src/objects/`)
- `heap.h` / `heap.cpp`, `objects.h` / `objects.cpp`

### 3.4 Ignition Interpreter & AST Compiler (`third_party/v8/src/interpreter/`)
- `bytecodes.h` / `bytecodes.cpp`, `compiler.h` / `compiler.cpp`, `interpreter.h` / `interpreter.cpp`

### 3.5 x86_64 JIT Assembler (`third_party/v8/src/codegen/x64/`)
- `assembler-x64.h` / `assembler-x64.cpp`, `jit-compiler-x64.h` / `jit-compiler-x64.cpp`

### 3.6 Public API & Adapter (`third_party/v8/src/api/` & `src/adapter/`)
- `api.h` / `api.cpp`, `atoms_v8_platform.h` / `atoms_v8_platform.cpp`

### 3.7 V8 Verification Suite & Test Runner (`third_party/v8/tests/`)
- `v8_test_suite.h` / `v8_test_suite.cpp`, `v8_test_main.cpp`

### 3.8 Build & Integration Files
- `BUILD.gn`, `kernel/apps/atrix/atrix_browser.c`, `build.ps1`

---

## 4. Risk Assessment & Safety Boundaries

- **W^X Memory Protection Risk:** Executing machine code in writable pages creates security vulnerabilities.
  - **Mitigation:** The JIT assembler strictly transitions pages to `PROT_READ | PROT_EXEC` via `sys_mprotect` before calling function pointers.
- **Garbage Collection Root Safety:** Unanchored object pointers may be collected during heap allocation.
  - **Mitigation:** Strict `HandleScope` RAII root tracking across all V8 API entry points.
