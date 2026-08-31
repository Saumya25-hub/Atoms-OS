# ATRIX Browser — Phase 7: Userspace Architecture & C/C++ Runtime Foundation

**Document ID:** ATRIX-ARCH-PHASE7-001  
**Phase:** Phase 7 — Final Architecture Documentation  
**Status:** Certified & Production Baseline  
**Date:** 2026-08-26  

---

## 1. Overview & Architectural Directives

Phase 7 establishes the userspace runtime environment for ATOMS OS, enabling the system to host modern, standard-compliant C and C++ runtimes required for large-scale open-source engines like Chromium, V8, Blink, and Skia.

In accordance with the **Open-Source First Directive**, ATOMS OS does not reinvent standard library primitives. Instead, it adapts upstream open-source components under permissive licenses (musl libc, LLVM libc++, and dlmalloc) and bridges them to the ATOMS OS kernel through a hardware-accelerated `IA32_LSTAR` system call gateway.

```text
┌────────────────────────────────────────────────────────────────────────┐
│               ATRIX Browser / Userspace Application (C++)              │
├───────────────────────────────────┬────────────────────────────────────┤
│         Open-Source C++20         │         Open-Source C11            │
│         Runtime (libc++)          │           libc (musl)              │
│  • std::string, std::vector       │  • malloc, free, calloc, realloc   │
│  • std::atomic, std::mutex        │  • printf, snprintf, puts          │
│  • std::chrono, std::unique_ptr   │  • pthreads & futex primitives     │
│  • operator new / delete          │  • open, read, write, close, seek  │
├───────────────────────────────────┴────────────────────────────────────┤
│                    ATOMS Userspace CRT0 / Syscall ABI                  │
│               (__atoms_syscall0 .. __atoms_syscall6)                   │
├────────────────────────────────────────────────────────────────────────┤
│          ATOMS Kernel IA32_LSTAR Syscall Dispatcher (Ring 0)           │
│     (sys_mmap, sys_munmap, sys_mprotect, sys_futex, sys_open, etc)     │
├────────────────────────────────────────────────────────────────────────┤
│               ATOMS Core Subsystems (VMM, PMM, Task, VFS)              │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Kernel Syscall Gateway Reference

| Syscall Number | Syscall Name | Parameters | Description |
|:---:|:---|:---|:---|
| **0** | `SYS_WRITE` | `(buf, len)` | Writes raw character stream to standard output / console. |
| **1** | `SYS_EXIT` | `(code)` | Terminates calling process or thread. |
| **2** | `SYS_GETPID` | `()` | Returns process ID of current task. |
| **3** | `SYS_YIELD` | `()` | Yields remaining quantum to next ready task in scheduler runqueue. |
| **4** | `SYS_UPTIME` | `()` | Returns system uptime in milliseconds. |
| **8** | `SYS_MMAP` | `(addr, len, prot, flags, fd, off)` | Allocates and maps anonymous physical pages into task's PML4. |
| **9** | `SYS_MUNMAP` | `(addr, len)` | Unmaps virtual page range and frees underlying physical frames. |
| **10** | `SYS_MPROTECT` | `(addr, len, prot)` | Modifies PTE read/write/NX permissions and flushes TLB. |
| **11** | `SYS_FUTEX` | `(uaddr, op, val, timeout)` | Fast userspace locking primitive (FUTEX_WAIT / FUTEX_WAKE). |
| **12** | `SYS_CLOCK_GETTIME`| `(clock_id, tp)` | Queries high-resolution monotonic time from RDTSC/APIC. |
| **13** | `SYS_NANOSLEEP`| `(req, rem)` | Puts current task to sleep for specified duration. |
| **14** | `SYS_OPEN` | `(path, flags, mode)` | Opens file descriptor in VFS. |
| **15** | `SYS_READ` | `(fd, buf, count)` | Reads bytes from open VFS file descriptor. |
| **25** | `SYS_CLOSE`| `(fd)` | Closes open VFS file descriptor. |
| **26** | `SYS_SEEK` | `(fd, offset, whence)` | Repositions file offset within open file. |
| **27** | `SYS_THREAD_SPAWN` | `(entry, stack_top, arg)` | Spawns user thread sharing parent address space. |
| **28** | `SYS_THREAD_EXIT` | `(exit_code)` | Terminates calling thread. |
| **29** | `SYS_WRITE_FILE` | `(fd, buf, count)` | Writes bytes to open VFS file descriptor. |

---

## 3. Provenance & License Summary

- **musl libc components:** MIT License (Rich Felker et al.)
- **LLVM libc++ components:** Apache 2.0 with LLVM Exception (LLVM Project)
- **dlmalloc memory allocator:** Public Domain / Permissive (Doug Lea)
- **Kernel Syscall Bridge & Verification Suite:** Proprietary / ATOMS OS Team

See [`ATOMS_THIRDPARTY_RUNTIME_LICENSES.md`](file:///D:/Signatures_OS/ATOMS_THIRDPARTY_RUNTIME_LICENSES.md), [`ATOMS_RUNTIME_PROVENANCE.md`](file:///D:/Signatures_OS/ATOMS_RUNTIME_PROVENANCE.md), and [`ATOMS_RUNTIME_COMPONENT_MAP.md`](file:///D:/Signatures_OS/ATOMS_RUNTIME_COMPONENT_MAP.md) for complete details.
