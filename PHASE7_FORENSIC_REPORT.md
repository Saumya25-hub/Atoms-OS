# PHASE 7 FORENSIC REPORT: USERSPACE & C/C++ RUNTIME FOUNDATION

**Document ID:** ATRIX-PHASE7-FORENSIC-001  
**Phase:** TASK 1 — FORENSIC INVESTIGATION  
**Target Subsystem:** ATOMS Userspace Architecture, Syscall Gateway, C/C++ Runtime & Chromium Prerequisites  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Executive Forensic Summary

A repository-wide forensic audit was conducted across `kernel/core/syscall/`, `kernel/core/memory/`, `kernel/core/process/`, `kernel/core/scheduler/`, `kernel/vfs/`, and `userspace/` to evaluate ATOMS OS's ability to host a standard C and C++ runtime environment required for Chromium embedder bring-up.

The audit revealed that while ATOMS OS possesses low-level kernel primitives (4-level paging VMM, PMM, hardware `IA32_LSTAR` syscall entry, preemptive scheduler, PCB process manager, and VFS), the userspace syscall layer and C/C++ runtime foundations are incomplete and disconnected from standard libc/libc++ expectations.

---

## 2. Forensic Audit Matrix (Existing Userspace Subsystems)

| Subsystem / Facility | Current Source Location | Status | Forensic Evidence & Findings |
|:---|:---|:---|:---|
| **Ring 3 CPU Support** | `arch/x86_64/`, `kernel/core/process/src/process.c` | **READY** | GDT descriptors `0x23` (User Code RPL 3) and `0x1B` (User Data RPL 3) configured; `process_spawn()` builds valid `iretq` frame with IF=1. |
| **Virtual Address Spaces** | `kernel/core/memory/vmm/` | **READY** | Isolated 4-level PML4 address spaces created per PCB with canonical 48-bit user boundaries (`0x01000000` to `0x7FFFFFFFFFFF`). |
| **CR3 Hardware Switching** | `kernel/core/memory/vmm/src/vmm.c` | **READY** | `vmm_switch_address_space()` safely loads CR3 during task context switches and restores kernel page tables on demand. |
| **Hardware Syscall Path** | `kernel/core/syscall/src/syscall_entry.asm` | **READY** | `IA32_LSTAR` MSR configured to `syscall_entry`; preserves full user context in `ATOMS_SyscallFrame` and validates user pointers. |
| **Syscall Dispatcher** | `kernel/core/syscall/src/dispatcher.c` | **PARTIAL** | Only dispatches 17 legacy/GUI syscalls (`SYS_WRITE`, `SYS_EXIT`, `SYS_ALLOC`, etc.). Missing standard memory, file, and sync syscalls. |
| **Userspace Memory (mmap)** | `kernel/core/syscall/src/services.c` | **MISSING** | `sys_service_alloc()` implements a crude 1-page bumped allocator (`0x40020000`); no `sys_mmap`, `sys_munmap`, or `sys_mprotect` exists. |
| **Thread Synchronization** | `kernel/core/sync/` | **MISSING** | Kernel has spinlocks and mutexes, but user-level `sys_futex` (`FUTEX_WAIT` / `FUTEX_WAKE`) is completely absent. |
| **Userspace Threads** | `kernel/core/scheduler/src/scheduler.c` | **PARTIAL** | `scheduler_create_user_task()` exists in kernel, but no syscall exists for a user process to spawn or exit worker threads. |
| **High-Resolution Clocks** | `kernel/core/timer/` | **PARTIAL** | Calibrated RDTSC and APIC timer exist in kernel, but `sys_clock_gettime` / `sys_nanosleep` are not exposed via syscalls. |
| **VFS File I/O Syscalls** | `kernel/vfs/vfs_legacy/` | **MISSING** | `vfs_open()`, `vfs_read()`, `vfs_write()`, `vfs_close()` exist in kernel, but are not wired into the active `dispatcher.c` table. |
| **Process Lifecycle** | `kernel/core/process/process_manager.c` | **READY** | PCB management with PID allocation, state tracking (`READY`, `RUNNING`, `ZOMBIE`, `TERMINATED`), and child enumeration fully functional. |
| **ELF Executable Loader** | `kernel/core/loader/elf/` | **READY** | `elf_load_image()` parses 64-bit ELF headers and program headers, mapping `PT_LOAD` segments into the process's PML4. |
| **Standard C Runtime** | `userspace/libbos/` | **STUB** | `libbos` contains ad-hoc wrappers using mismatched syscall numbers; no standard POSIX `libc` (memory allocation, stdio, string) exists. |
| **Standard C++ Runtime** | None | **MISSING** | No C++ runtime (`libc++`), `operator new`/`delete`, RTTI/typeinfo, `std::string`, `std::vector`, or `std::mutex` exists in userspace. |

---

## 3. Open-Source Component Investigation & Candidate Evaluation

In accordance with the **Open-Source First Directive**, mature open-source C and C++ runtime projects were evaluated:

### 3.1 C Standard Library (`libc`) Candidates
1. **musl libc** (Version 1.2.5, MIT License, Rich Felker et al.):
   - *Pros:* Ultra-clean code, highly standard-compliant, explicit `__syscall()` architecture, zero GNU bloat, ideal for custom OS kernels.
   - *Verdict:* **RECOMMENDED AS PRIMARY LIBC ARCHITECTURE**.
2. **Newlib** (Red Hat / BSD / GPL):
   - *Cons:* Heavy configure scripts, cumbersome build system.
   - *Verdict:* REJECTED in favor of musl.
3. **dlmalloc / TLSF Allocator** (Doug Lea / Public Domain / 2-Clause BSD):
   - *Pros:* Battle-tested, rock-solid userspace heap allocator operating directly on top of `mmap()` / `munmap()` anonymous pages.
   - *Verdict:* **RECOMMENDED FOR USERSPACE HEAP**.

### 3.2 C++ Standard Library & ABI (`libc++`) Candidates
1. **LLVM libc++ / libc++abi** (Apache-2.0 with LLVM Exception, The LLVM Project):
   - *Pros:* Fully supports Clang 19, C++20 standards, clean `-fno-exceptions` and `-fno-rtti` modes as required by Chromium, highly modular headers.
   - *Verdict:* **RECOMMENDED AS PRIMARY C++ RUNTIME ARCHITECTURE**.
2. **GCC libstdc++** (GPLv3 with Runtime Exception):
   - *Cons:* Tightly coupled to GCC internals; potential copyleft ambiguity.
   - *Verdict:* REJECTED in favor of LLVM libc++.

---

## 4. Root Causes & Blockers to Resolve

1. **Kernel Syscall Table Deficit:** Standard runtimes require POSIX-aligned syscall numbers and functions for virtual memory (`mmap`, `munmap`, `mprotect`), synchronization (`futex`), time (`clock_gettime`, `nanosleep`), file descriptors (`open`, `read`, `write`, `close`, `lseek`), and threading (`thread_spawn`, `thread_exit`).
2. **Heap Memory Disconnect:** Userspace `malloc()` requires a clean `mmap()` backend to allocate multi-page heap arenas from the kernel VMM rather than single fixed-address buffers.
3. **Userspace Runtime Separation:** Userspace needs a dedicated `userspace/runtime/` layer housing the adapted open-source C library headers, C++ runtime primitives (`<new>`, `<typeinfo>`, `<vector>`, `<string>`, `<atomic>`, `<mutex>`, `<chrono>`), and entry-point crt0 (`_start`).

---

## 5. Risk Analysis & Safety Boundaries

- **User Pointer Security:** Every new syscall MUST validate that user pointers reside strictly within user space (`0x0000000001000000` to `0x00007FFFFFFFFFFF`) to prevent kernel memory corruption or privilege escalation.
- **Page Table Isolation:** Anonymous memory allocated via `sys_mmap` must belong strictly to the calling task's PML4 (`current->pml4`) with `PAGE_USER` permissions, and must be freed cleanly upon process termination.
- **Zero Kernel Regression:** All existing certified stages (Phase 1 UI, Phase 2 Networking, Phase 3 TLS 1.2, Phase 4 HTML5, Phase 5 CSSOM) must remain 100% operational.
