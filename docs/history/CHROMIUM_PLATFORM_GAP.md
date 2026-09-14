# CHROMIUM ↔ ATOMS OS PLATFORM GAP ANALYSIS & ADAPTER SPECIFICATION

**Document ID:** ATRIX-PHASE6-GAP-001  
**Phase:** Phase 6 — Forensic Platform Gap Analysis  
**Date:** 2026-08-26  

---

## 1. Comprehensive Gap Classification

| Category | Primitives Missing in ATOMS | Impact on Chromium | Remediation / Adapter Strategy | Complexity |
|:---|:---|:---|:---|:---|
| **Memory Syscalls** | `sys_mmap`, `sys_munmap`, `sys_mprotect` | **CRITICAL** (Required for V8 JIT, PartitionAlloc, WebAssembly) | Add 3 syscalls to `kernel/core/syscall/` wrapping `vmm_alloc_mapped_page()` and `vmm_unmap_page()`. | **MEDIUM** |
| **C / C++ Userspace Runtime** | POSIX `libc` (`musl`), `libc++`, `libunwind` | **BLOCKER** (Chromium requires C++20 standard library) | Compile a lightweight, unprivileged `musl libc` + LLVM `libc++` userspace runtime library for ATOMS. | **HIGH** |
| **Synchronization** | `sys_futex`, `pthread_mutex`, `pthread_cond` | **CRITICAL** (Multi-threaded V8 GC, Chromium task runners) | Implement kernel `sys_futex` with `FUTEX_WAIT` and `FUTEX_WAKE` operations. | **MEDIUM** |
| **Inter-Process Communication** | POSIX file descriptor passing (`sendmsg` with `SCM_RIGHTS`), POSIX pipes | **HIGH** (Mojo IPC multi-process mode) | **Short Term:** Run Chromium in single-process mode (`--single-process`).<br>**Long Term:** Build ATOMS Mojo channel adapter over `kernel/ipc/`. | **HIGH** |
| **Filesystem Interface** | POSIX `open`, `read`, `write`, `close`, `lseek`, `stat`, `mkdir` | **MEDIUM** (Storage, cookies, cache, local files) | Wire userland `musl` file descriptor operations directly to ATOMS VFS syscalls (`vfs_open`, `vfs_read`). | **MEDIUM** |
| **Socket Syscalls** | POSIX `socket`, `connect`, `send`, `recv`, `select`/`epoll` | **MEDIUM** (Chromium `net/` HTTP/TLS networking) | Expose kernel socket subsystem (`kernel/net/socket/`) via user-mode BSD socket syscalls. | **MEDIUM** |
| **Font Services** | Fontconfig / DirectWrite font matching | **LOW** (System font discovery) | Bundle standard Web fonts (`Roboto`, `OpenSans`, `DejaVuSans`) and load directly via FreeType. | **LOW** |
| **Hardware Sandboxing** | Linux User Namespaces / Seccomp BPF / Windows Job Objects | **OPTIONAL** (Process security isolation) | Run with `--no-sandbox` during bring-up; implement ATOMS capability-based token sandbox in Phase 16. | **HIGH** |
| **Build Toolchain** | GN (Generate Ninja) toolchain for ATOMS OS | **BLOCKER** (Chromium cannot be built with `build.ps1`) | Create a custom `toolchain("atoms_x64")` GN configuration file utilizing Clang cross-compiler. | **HIGH** |

---

## 2. The ATOMS Platform Adapter Layer (APAL) Architecture

To bridge Chromium cleanly without hacking Chromium core source files, we define the **ATOMS Platform Adapter Layer (APAL)**:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        CHROMIUM CORE SOURCE                            │
│           (Blink / V8 / Skia / Mojo / Network Service)                 │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                 ATOMS PLATFORM ADAPTER LAYER (APAL)                    │
├─────────────────────────┬──────────────────────────────────────────────┤
│ APAL Memory Interface   │ Implements base::PageAllocator via mmap      │
├─────────────────────────┼──────────────────────────────────────────────┤
│ APAL Task Runner        │ Dispatches tasks to ATOMS thread scheduler   │
├─────────────────────────┼──────────────────────────────────────────────┤
│ APAL Mojo Channel       │ Bridges Mojo pipes to ATOMS kernel IPC       │
├─────────────────────────┼──────────────────────────────────────────────┤
│ APAL Skia Blitter       │ Copies SkBitmap pixel rects to BWE Surfaces  │
├─────────────────────────┼──────────────────────────────────────────────┤
│ APAL Input Gateway      │ Translates BOS_GUIEvent to WebInputEvent     │
├─────────────────────────┼──────────────────────────────────────────────┤
│ APAL Network Transport  │ Bridges Chromium net/ to ATOMS TCP/IP stack  │
└─────────────────────────┴──────────────────┬───────────────────────────┘
                                             │
                                             ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        ATOMS OS KERNEL & BWE                           │
└────────────────────────────────────────────────────────────────────────┘
```
