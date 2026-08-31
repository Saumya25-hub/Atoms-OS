# PHASE 7 PATCH PLAN: USERSPACE & C/C++ RUNTIME FOUNDATION

**Document ID:** ATRIX-PHASE7-PLAN-001  
**Phase:** TASK 2 — ARCHITECT TEAM  
**Target Subsystem:** ATOMS Userspace Architecture, Syscall Gateway, C/C++ Runtime & Chromium Prerequisites  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Architectural Scope & Target Pipeline

Phase 7 establishes the authoritative, open-source-adapted userspace C and C++ runtime foundation for ATOMS OS. The target runtime stack is:

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                     USERSPACE APPLICATION (C / C++)                     │
│                 (e.g., ATRIX Chromium Embedder / Tests)                 │
├───────────────────────────────────┬─────────────────────────────────────┤
│         Open-Source C++20         │          Open-Source libc           │
│         Runtime (libc++)          │           (musl-adapted)            │
│  (<new>, <string>, <vector>, etc) │  (malloc, printf, open, read, etc)  │
├───────────────────────────────────┴─────────────────────────────────────┤
│                   ATOMS Userspace CRT0 / Syscall ABI                    │
│             (__syscall6, entrypoint _start, TLS initialization)         │
├─────────────────────────────────────────────────────────────────────────┤
│            ATOMS Kernel Syscall Gateway (IA32_LSTAR / CPL 0)            │
│   (sys_mmap, sys_munmap, sys_mprotect, sys_futex, sys_open, etc)       │
├─────────────────────────────────────────────────────────────────────────┤
│            ATOMS Core Subsystems (VMM, PMM, Scheduler, VFS)             │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Files to Create and Modify

### 2.1 Kernel Syscall Gateway Subsystem
- **Modify:** [`kernel/core/syscall/include/syscall.h`](file:///D:/Signatures_OS/kernel/core/syscall/include/syscall.h)
  - Define standard syscall constants: `SYS_MMAP` (8), `SYS_MUNMAP` (9), `SYS_MPROTECT` (10), `SYS_FUTEX` (11), `SYS_CLOCK_GETTIME` (12), `SYS_NANOSLEEP` (13), `SYS_OPEN` (14), `SYS_READ` (15), `SYS_CLOSE` (25), `SYS_SEEK` (26), `SYS_THREAD_SPAWN` (27), `SYS_THREAD_EXIT` (28).
  - Define `PROT_READ` (1), `PROT_WRITE` (2), `PROT_EXEC` (4), `PROT_NONE` (0).
  - Define `MAP_PRIVATE` (2), `MAP_ANONYMOUS` (0x20), `MAP_FIXED` (0x10).
  - Define `FUTEX_WAIT` (0), `FUTEX_WAKE` (1).
- **Modify:** [`kernel/core/syscall/src/dispatcher.c`](file:///D:/Signatures_OS/kernel/core/syscall/src/dispatcher.c)
  - Route newly defined syscall IDs to their respective service functions in `services.c`.
- **Modify:** [`kernel/core/syscall/src/services.c`](file:///D:/Signatures_OS/kernel/core/syscall/src/services.c)
  - Implement `sys_service_mmap()`: allocates anonymous pages via `pmm_alloc_page()` and maps them into `current->pml4` using `vmm_map_page()`.
  - Implement `sys_service_munmap()`: unmaps virtual addresses and frees physical pages via `vmm_unmap_page()` and `pmm_free_page()`.
  - Implement `sys_service_mprotect()`: modifies PTE protection flags (`PAGE_WRITABLE`, `VMM_FLAG_NX`) and invalidates TLB.
  - Implement `sys_service_futex()`: provides thread sleep/wake synchronization on 32-bit user memory addresses.
  - Implement `sys_service_clock_gettime()`: queries calibrated monotonic time from RDTSC/APIC.
  - Implement `sys_service_nanosleep()`: puts current task to sleep for the requested duration.
  - Implement `sys_service_open()`, `sys_service_read()`, `sys_service_close()`, `sys_service_seek()`: routes to ATOMS VFS.
  - Implement `sys_service_thread_spawn()`: creates a user task sharing the parent's PML4 and binds it to the scheduler.

### 2.2 Userspace Standard C Runtime (`userspace/runtime/c/`)
- **Create:** `userspace/runtime/c/include/syscall.h`: Assembly `__syscall()` helpers for x86_64.
- **Create:** `userspace/runtime/c/include/stddef.h`, `stdint.h`, `stdbool.h`, `stdlib.h`, `stdio.h`, `string.h`, `unistd.h`, `sys/mman.h`, `sys/time.h`, `time.h`, `pthread.h`, `sched.h`.
- **Create:** `userspace/runtime/c/src/crt0.asm`: Standard user application entry point (`_start`) initializing arguments, stack alignment, and calling `main()`.
- **Create:** `userspace/runtime/c/src/memory.c`: Userspace `malloc()`, `free()`, `calloc()`, `realloc()` allocator operating on top of `mmap()`.
- **Create:** `userspace/runtime/c/src/stdio.c`: `printf()`, `snprintf()`, `puts()`, `putchar()` formatting.
- **Create:** `userspace/runtime/c/src/string.c`: Standard string manipulation routines.
- **Create:** `userspace/runtime/c/src/pthread.c`: User-mode `pthread_create()`, `pthread_join()`, `pthread_mutex_init()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()`, `pthread_cond_init()`, `pthread_cond_wait()`, `pthread_cond_signal()`.

### 2.3 Userspace Standard C++ Runtime (`userspace/runtime/cpp/`)
- **Create:** `userspace/runtime/cpp/include/new`: Standard C++ memory allocation operators (`operator new`, `operator delete`, `operator new[]`, `operator delete[]`, placement new, `std::nothrow`).
- **Create:** `userspace/runtime/cpp/include/typeinfo`: RTTI and `std::type_info` definition.
- **Create:** `userspace/runtime/cpp/include/utility`, `type_traits`, `memory`, `algorithm`, `initializer_list`.
- **Create:** `userspace/runtime/cpp/include/atomic`: C++20 `std::atomic` wrapping compiler atomic builtins.
- **Create:** `userspace/runtime/cpp/include/mutex`: C++20 `std::mutex`, `std::lock_guard`, `std::unique_lock` based on futex/pthread.
- **Create:** `userspace/runtime/cpp/include/chrono`: `std::chrono::system_clock`, `steady_clock`, `high_resolution_clock`.
- **Create:** `userspace/runtime/cpp/include/string`: C++20 `std::basic_string` and `std::string`.
- **Create:** `userspace/runtime/cpp/include/vector`: C++20 `std::vector` dynamic container.
- **Create:** `userspace/runtime/cpp/src/cxx_runtime.cpp`: Global operators `new`/`delete`, pure virtual function handler (`__cxa_pure_virtual`), and exception stubs.

### 2.4 Deterministic Test Suite (`userspace/tests/runtime_test/`)
- **Create:** `userspace/tests/runtime_test/runtime_test_suite.c` & `.cpp`: Implements all 20 deterministic tests verifying C and C++ runtime capabilities.

---

## 3. Risk Assessment & Rollback Plan

- **Risk:** Malformed user pointers could cause kernel page faults.
  - **Mitigation:** Strict `syscall_validate_user_ptr()` bounds checks on all addresses passed from userspace.
- **Risk:** Heap fragmentation or memory leaks in long-running user processes.
  - **Mitigation:** The userspace memory allocator will use bucketed size classes and coalesce freed blocks before returning unused pages to the kernel via `munmap()`.
- **Rollback:** In the event of any kernel instability, the syscall additions in `dispatcher.c` can be bypassed without affecting existing subsystems.
