# ATOMS OS RUNTIME PROVENANCE & SOURCE ATTRIBUTION

**Document ID:** ATRIX-PHASE7-PROVENANCE-001  
**Phase:** Phase 7 — Source Code Provenance Record  
**Date:** 2026-08-26  

---

## 1. Classification Definitions

Every component in the ATOMS OS Userspace & C/C++ Runtime stack is strictly categorized into one of four distinct ownership classes:

1. **ATOMS ORIGINAL CODE:** Code authored directly for ATOMS OS without external reference (Kernel syscall gateway, VMM memory bridge, BWE window blitter, hardware drivers).
2. **OPEN-SOURCE THIRD-PARTY CODE:** Unmodified open-source code obtained from upstream projects.
3. **ADAPTED CODE:** Open-source code adapted to interface with ATOMS OS system call ABIs and memory layouts while preserving upstream core logic and licensing headers.
4. **ATOMS INTEGRATION CODE:** Glue layers, header bridges, and wrappers connecting third-party libraries to ATOMS kernel interfaces.

---

## 2. Detailed Provenance Breakdown

| Runtime Module | Source File(s) | Category | Upstream Project / Author | License |
|:---|:---|:---|:---|:---|
| **Syscall Gateway & Handlers** | `kernel/core/syscall/` | **ATOMS ORIGINAL CODE** | Saumya Chaudhari / ATOMS Team | Proprietary / ATOMS OS License |
| **PML4 Virtual Memory Management** | `kernel/core/memory/vmm/` | **ATOMS ORIGINAL CODE** | Saumya Chaudhari / ATOMS Team | Proprietary / ATOMS OS License |
| **Preemptive Task Scheduler** | `kernel/core/scheduler/` | **ATOMS ORIGINAL CODE** | Saumya Chaudhari / ATOMS Team | Proprietary / ATOMS OS License |
| **C Runtime Architecture & Headers** | `userspace/runtime/c/include/` | **ADAPTED CODE** | musl libc / Rich Felker et al. | MIT License |
| **Userspace Memory Allocator** | `userspace/runtime/c/src/memory.c` | **ADAPTED CODE** | dlmalloc / Doug Lea | Public Domain / Permissive |
| **Pthread & Futex Synchronization** | `userspace/runtime/c/src/pthread.c` | **ATOMS INTEGRATION CODE** | Adapted from musl pthread logic | MIT / ATOMS Integration |
| **Standard I/O & Formatted Printf** | `userspace/runtime/c/src/stdio.c` | **ADAPTED CODE** | musl libc / Rich Felker | MIT License |
| **C++ Operators (`new`/`delete`)** | `userspace/runtime/cpp/src/` | **ADAPTED CODE** | LLVM libc++ / libc++abi | Apache 2.0 with LLVM Exception |
| **C++ Containers (`std::string`, `std::vector`)** | `userspace/runtime/cpp/include/` | **ADAPTED CODE** | LLVM libc++ project | Apache 2.0 with LLVM Exception |
| **C++ Atomics & Mutex Primitives** | `userspace/runtime/cpp/include/` | **ADAPTED CODE** | LLVM libc++ project | Apache 2.0 with LLVM Exception |
| **Runtime Test Suite** | `userspace/tests/runtime_test/` | **ATOMS ORIGINAL CODE** | Saumya Chaudhari / ATOMS Team | Proprietary / ATOMS OS License |
