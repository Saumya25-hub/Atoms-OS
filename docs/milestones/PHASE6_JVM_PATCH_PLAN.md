# ATOMS OS — Java Runtime Phase 6 Patch Plan

## 1. Objective
* **Milestone**: Phase 6 — JIT Compilation + Java Execution Performance
* **Protocol**: ATOMS OS Engineering Protocol V1 — Rule 0 (Task 2)
* **Goal**: Specify all exact files, structures, algorithms, and test assets to be added or modified for the JIT compiler implementation.

---

## 2. Components to Implement

### A. JIT Header Definitions ([`third_party/avian/include/avian/jit.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/jit.h))
* Define `class JitCompiler` and `class CodeCache`.
* Define method compilation signature:
  `typedef int64_t (*CompiledMethodPtr)(int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3);`
* Define JIT status codes: `JitSuccess`, `JitUnsupportedOpcode`, `JitOutOfMemory`, `JitFallback`.

### B. JIT Engine & x86_64 Code Generation ([`third_party/avian/src/jit.cpp`](file:///D:/Signatures_OS/third_party/avian/src/jit.cpp))
* **x86_64 Assembler primitives**:
  - `push rbp`, `mov rbp, rsp`, `sub rsp, <frame_size>`
  - Register assignment for locals (`rdi`, `rsi`, `rdx`, `rcx` -> stack spill or register slots)
  - Arithmetic encoding: `add eax, ebx`, `sub eax, ebx`, `imul eax, ebx`, `idiv ebx`, `lea eax, [rax*2+rax]`, etc.
  - Branch encoding: `cmp eax, ebx`, `je <rel8/32>`, `jne`, `jl`, `jge`, `jle`, `jg`, `jmp <rel8/32>`
  - Epilogue: `mov rsp, rbp`, `pop rbp`, `ret`
* **W^X Memory Management**:
  - `CodeCache::allocate(size_t size)` allocates via `mmap(PROT_READ | PROT_WRITE)`.
  - `CodeCache::makeExecutable(void* ptr, size_t size)` applies `mprotect(PROT_READ | PROT_EXEC)`.

### C. Processor Hook & Hot Method Detection ([`third_party/avian/src/processor.cpp`](file:///D:/Signatures_OS/third_party/avian/src/processor.cpp))
* Add `invocation_count` tracking in `Method` / `Processor`.
* When `invocation_count >= HOT_THRESHOLD` (e.g., 5 invocations or hot loop back-edge), invoke `JitCompiler::compileMethod()`.
* If compiled, dispatch directly to `CompiledMethodPtr`.
* If compilation fails, transparently fall back to bytecode interpreter loop.

### D. Verification & Benchmark Test Suite ([`userspace/apps/java/jvm_main.cpp`](file:///D:/Signatures_OS/userspace/apps/java/jvm_main.cpp))
* Add Phase 6 JIT verification suite:
  1. `JitTest`: Hot arithmetic calculation (`(x * 3) + 7` 100,000 times) with diagnostic proof `[JIT] compiling calculate()`.
  2. `JitFibTest`: Recursive/iterative Fibonacci calculation in JIT mode.
  3. `JitExceptionTest`: Safe exception handling and unwind from JIT-compiled method.
  4. `JitDifferentialTest`: Dual execution (JIT OFF vs JIT ON) ensuring 100% identical outputs.
  5. `JitFailureInjectionTest`: Forced fallback on unsupported opcode with zero crashes.
  6. `JitBenchmark`: Microbenchmark measuring compile overhead vs steady-state execution speedup.

---

## 3. Rollback & Risk Analysis
* **Risk**: Invalid instruction encoding causing GPF/SIGSEGV in Ring 3.
* **Mitigation**: Pure Ring 3 userspace execution; all opcodes verified via strict bounds-checked emitter; immediate fallback to interpreter on any unrecognized pattern.
* **Kernel Impact**: Zero. Kernel code is untouched.
