# CHROMIUM / ATOMS OS PROCESS & IPC GAP ANALYSIS

**Document ID:** ATOMS-CHROMIUM-PROC-IPC-001  
**Phase:** 16-B (Chromium Process + IPC + Exception Model Bring-Up)  
**Standard:** Strict Upstream Provenance, Zero Fake Tests, Phase Isolation Protocol  
**Date:** 2026-09-07  
**Status:** AUDIT COMPLETE — ADAPTATION ARCHITECTED  

---

## 1. Executive Summary

Phase 16-A successfully established the genuine upstream Google Chromium base compilation pipeline (`libatoms_base.a`, 4.8MB archive of genuine upstream `.cc` objects compiled via GN + Ninja).

Phase 16-B connects Chromium's real multi-process architecture (Browser, Renderer, Utility, GPU) to the native BOS Kernel process manager, scheduler, IPC engine, shared-memory subsystem, and exception handler through the ATOMS Platform Adaptation Layer (APAL).

This document establishes the forensic baseline of existing ATOMS OS kernel primitives, maps them 1:1 against genuine Chromium abstractions, identifies all operational gaps, and defines the minimal, correct userspace/kernel adaptation path without introducing Linux kernel code, custom browser architectures, or mock IPC buses.

---

## 2. Existing ATOMS OS Infrastructure Audit

| Subsystem / Capability | Existing ATOMS Implementation | File Location | Status & Characteristics |
|:---|:---|:---|:---|
| **Process Creation** | `ATOMS_Process_Create()`, `BOSX_LoadFromVFS()` | `kernel/core/process/process_manager.c`, `kernel/core/loader/bosx_loader.c` | Allocates PID (200–65535), creates isolated PML4 address space, allocates 128KB stack (`0x7FE00000ULL`–`0x7FE20000ULL`), parses BOSX binary, maps sections with W^X enforcement. |
| **Ring 3 Execution** | `process_spawn()` | `kernel/core/process/src/process.c` | Builds user interrupt frame (CS=0x23, SS=0x1B, RFLAGS=0x202, RIP=entry, RSP=stack_top), executes `iretq` into Ring 3 (CPL 3). |
| **Process Termination** | `ATOMS_Process_Terminate()` | `kernel/core/process/process_manager.c:230` | Transitions state to `TERMINATED` or `ZOMBIE`, records exit code and exit timestamp, closes desktop surfaces, terminates tasks via `scheduler_terminate_tasks_by_pid()`. |
| **PID Allocation** | `ATOMS_PID_Alloc()`, `ATOMS_PID_Free()` | `kernel/core/process/process_manager.c:50-85` | Monotonic counter with wrap-around, range 200–65535, bitmap reservation, conflict rejection. |
| **Address Space Creation** | `vmm_create_address_space()` | `kernel/core/memory/vmm/src/vmm.c:237` | Clones higher-half kernel mappings (`0xFFFF800000000000ULL+`), allocates clean PML4 table, isolates lower-half usermode space (`[0x40000000, 0x80000000)`). |
| **Address Space Destruction**| `vmm_destroy_address_space()` | `kernel/core/memory/vmm/src/vmm.c:280` | Walks user PML4 entries, frees all allocated PT, PD, PDP tables and mapped physical frames back to PMM. |
| **Thread Creation** | `sys_service_thread_spawn()`, `scheduler_create_user_task()` | `kernel/core/syscall/src/services.c:777`, `kernel/core/scheduler/src/scheduler.c` | Spawns execution context sharing parent process PML4 and PID, registers in `ATOMS_Process_RegisterThread()`. |
| **Thread Teardown** | `sys_service_thread_exit()`, `scheduler_terminate_task()` | `kernel/core/syscall/src/services.c:786` | Cleans thread context, reclaims stack, drops ref count. |
| **Process Reaping / Wait** | `ATOMS_Process_Wait()`, `ATOMS_Process_Reap()` | `kernel/core/process/process_manager.c:433, 279` | Waits for child process zombie state, captures exit code, clears PCB, decrements active process counter. |
| **IPC Channels & Queues** | `bos_ipc_create_channel()`, `bos_ipc_send()`, `bos_ipc_receive()`, `bos_ipc_close()` | `kernel/ipc/channels/channel_manager.c`, `kernel/ipc/core/ipc_manager.c` | In-kernel circular FIFO queues (`ipc_message_t`), capacity 128 messages, handles cross-PID messages up to 4096 bytes. |
| **Pipes** | `bos_pipe_create()`, `bos_pipe_write()`, `bos_pipe_read()` | `kernel/ipc/pipes/pipe_engine.c` | Unidirectional streaming byte ring buffers between reader PID and writer PID. |
| **Shared Memory** | `bos_shm_create()`, `bos_shm_open()`, `bos_shm_map()`, `bos_shm_unmap()` | `kernel/ipc/shared_memory/shm_manager.c` | Allocates contiguous physical pages (`pmm_alloc_page()`), maps identical physical frames into separate virtual address spaces (`vmm_map_page()`) with R/W permissions. |
| **Synchronization / Futex**| `sys_service_futex()` | `kernel/core/syscall/src/services.c:608` | `FUTEX_WAIT`, `FUTEX_WAKE`, `FUTEX_REQUEUE` syscalls for Ring 3 mutex/condition variable synchronization. |
| **Syscall Gateway** | MSR `IA32_LSTAR` + `syscall_entry.asm` + `syscall_dispatch()` | `kernel/core/syscall/src/` | Hardware fast syscall (`syscall`/`sysret`), ABI frame with 6 arguments, strict user pointer and RFLAGS sanitization. |
| **Exception & Fault Handling** | `exception_dispatch()` | `kernel/core/interrupt/src/exception.c:40` | Checks `(regs->cs & 0x03) == 0x03`. If Ring 3: logs fault pagewalk to COM1, terminates *only* faulting process (`ATOMS_Process_Terminate(pid, -vector)`), schedules next runnable task. Kernel and other processes remain 100% operational. |

---

## 3. Chromium Process Model Audit

Chromium organizes execution across multiple specialized processes to ensure security, stability, and responsiveness:

```text
                           ┌────────────────────────────────┐
                           │         Browser Process        │
                           │   (UI, Storage, Network, Mojo) │
                           └───────────────┬────────────────┘
                                           │
                  ┌────────────────────────┼────────────────────────┐
                  ▼                        ▼                        ▼
     ┌────────────────────────┐┌────────────────────────┐┌────────────────────────┐
     │    Renderer Process    ││    Renderer Process    ││      GPU Process       │
     │  (Blink, V8, Layout)   ││  (Blink, V8, Layout)   ││  (BGL, Skia GL, Audio) │
     │  [Sandboxed Ring 3]    ││  [Sandboxed Ring 3]    ││  [Sandboxed Ring 3]    │
     └────────────────────────┘└────────────────────────┘└────────────────────────┘
```

### 3.1 Chromium Abstractions vs ATOMS Implementation Mapping

| Genuine Chromium Class / File | Architectural Purpose | ATOMS Mapping | Syscall / APAL Target |
|:---|:---|:---|:---|
| `base::Process` (`base/process/process.h`, `process_posix.cc`) | Platform-independent process handle, lifecycle, termination, waiting | Maps to `apal_process_t` and ATOMS PID | `SYS_WAITPID` (38), `SYS_KILL` (41), `SYS_PROCESS_STATUS` (42) |
| `base::ProcessHandle` (`base/process/process_handle.h`) | OS-level process identifier (`pid_t`) | `typedef uint32_t apal_pid_t` (matches `pid_t`) | `SYS_GETPID` (2) |
| `base::LaunchProcess()` (`base/process/launch.h`, `launch_posix.cc`) | Spawns child process with specified arguments, environment, and file descriptors | Executes binary via VFS loader | `SYS_EXEC` (37) |
| `base::KillProcess()` (`base/process/kill.h`, `kill_posix.cc`) | Terminates target process by PID | Calls kernel process termination | `SYS_KILL` (41) |
| `base::GetTerminationStatus()` (`base/process/kill.h`) | Queries whether process exited normally or crashed | Queries kernel PCB state & exit vector | `SYS_WAITPID` (38) / `SYS_PROCESS_STATUS` (42) |
| `mojo::PlatformChannel` (`mojo/public/cpp/platform/platform_channel.h`) | Encapsulates connected bidirectional IPC endpoints for bootstrapping Mojo | Maps to ATOMS kernel IPC channel endpoints | `SYS_IPC_CALL` (39) / `apal_ipc_pipe_create()` |
| `mojo::core::Channel` (`mojo/core/channel.h`, `channel_posix.cc`) | I/O abstraction reading/writing delimited Mojo messages | Uses ATOMS IPC packet delivery or pipe buffers | `SYS_IPC_CALL` (39) |
| `base::subtle::PlatformSharedMemoryRegion` (`base/memory/platform_shared_memory_region.h`) | Allocates and shares physical pages across processes | Uses ATOMS in-kernel SHM manager | `SYS_SHM_CALL` (40) |

---

## 4. Gap Analysis & Required Adaptations

### Gap 1: Exposing Child Process Wait/Reap to Userspace
- **Current State:** `ATOMS_Process_Wait(pid, &exit_code)` exists in `kernel/core/process/process_manager.c:433`, but there is no syscall entry point for Ring 3 parent processes to wait on child termination.
- **Required Adaptation:** Implement `sys_service_waitpid` (Syscall 38) and wire it into `syscall_dispatch()` in `kernel/core/syscall/src/dispatcher.c`.
- **Chromium Consumer:** `base::Process::WaitForExitWithTimeout()`, `base::GetTerminationStatus()`.

### Gap 2: Exposing Process Termination / Signaling to Userspace
- **Current State:** `ATOMS_Process_Terminate(pid, exit_code)` exists in kernel, but userspace cannot terminate an errant or hanging child process.
- **Required Adaptation:** Implement `sys_service_kill` (Syscall 41) allowing a parent process to terminate a child.
- **Chromium Consumer:** `base::Process::Terminate()`, `base::KillProcess()`.

### Gap 3: Cross-Process IPC Endpoints via Syscall Gateway
- **Current State:** `kernel/ipc/` provides `bos_ipc_create_channel()`, `bos_ipc_send()`, `bos_ipc_receive()`, `bos_ipc_close()`. However, `atoms/userspace/apal/ipc/` used an in-process ring buffer that could not cross process boundaries.
- **Required Adaptation:** Implement `sys_service_ipc_call` (Syscall 39) that exposes `bos_ipc_create_channel`, `bos_ipc_connect`, `bos_ipc_send`, `bos_ipc_receive`, and `bos_ipc_close` directly to userspace.
- **Chromium Consumer:** `mojo::PlatformChannel`, `mojo::core::Channel`.

### Gap 4: Cross-Process Shared Memory Mapping
- **Current State:** `kernel/ipc/shared_memory/shm_manager.c` allocates physical pages and maps them into PML4 tables. In `sys_service_mmap`, `MAP_SHARED` allocates fresh pages per call.
- **Required Adaptation:** Implement `sys_service_shm_call` (Syscall 40) exposing `bos_shm_create`, `bos_shm_open`, `bos_shm_map` (mapping to user window `0x60000000+` with `PAGE_USER`), and `bos_shm_unmap`.
- **Chromium Consumer:** `base::WritableSharedMemoryRegion`, `base::ReadOnlySharedMemoryRegion`, Mojo shared buffers.

### Gap 5: ABDE Diagnostic Isolation on Usermode Faults
- **Current State:** In `kernel/core/interrupt/src/exception.c:52-54`, `diag_set_fail("IDT")` is called unconditionally on all exceptions before checking if the fault occurred in Ring 3.
- **Required Adaptation:** Guard `diag_set_fail("IDT")` so it is only triggered when `(regs->cs & 0x03) != 0x03` (CPL 0 kernel fault). For CPL 3, preserve clean ABDE status while isolating the faulting process.
- **Chromium Consumer:** Renderer crash isolation without kernel panic.

---

## 5. Security & Safety Architectural Assurances

1. **W^X Enforcement:** User pages are either Writable or Executable, never both.
2. **Usermode Window Bounds:** All user pointers and mappings are strictly bounded within `[0x40000000, 0x80000000)`. Any attempt to map kernel space (`0xFFFF800000000000ULL+`) or framebuffer (`0x90000000+`) from Ring 3 is unconditionally rejected.
3. **No Linux Kernel Imports:** All adaptations use ATOMS OS native `ATOMS_PCB`, `vmm_map_page`, `pmm_alloc_page`, and `bos_ipc_*` facilities.
4. **Binary Compatibility:** Existing BOFS, GUI, and Phase 7/13 certification dashboards remain completely untouched and certified.
