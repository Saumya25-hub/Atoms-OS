# ATOMS OS — PHASE 1 BOS USERLAND RUNTIME FOUNDATION
## TASK 1: FORENSIC INVESTIGATION REPORT

**Target Subsystems**: Memory, Threading, TLS, Synchronization, Filesystem, Runtime Primitives  
**Protocol Phase**: TASK 1 (Forensic Investigation — Zero Code Modifications)  
**Date**: September 14, 2026  
**Investigator**: ATOMS Forensic Team  

---

### 1. Executive Forensic Summary

A forensic re-audit of the BOS Kernel and userspace runtime was executed to verify the Phase 0 audit findings and establish the technical baseline required for hosting an embedded Java Virtual Machine runtime (Avian).

The investigation verified that while the BOS Kernel provides a 64-bit Long Mode execution environment, isolated PML4 address spaces, pre-emptive multi-core scheduling, and native BOFS filesystem services, six critical runtime gaps prevent it from currently hosting a multi-threaded virtual machine or C++ runtime:
1. **Thread-Local Storage (TLS) Loss Across Context Switches**: The hidden 64-bit base address of `%fs` (`IA32_FS_BASE` MSR 0xC0000100) is neither saved nor restored during task switching, and segment register reloads unconditionally reset the base to zero.
2. **Cooperative Busy-Loop Futex**: `SYS_FUTEX` (`FUTEX_WAIT`) only yields without blocking, causing 100% CPU starvation during lock contention, while `FUTEX_WAKE` performs no scheduler wakeups.
3. **Restricted Bump-Allocated `mmap` with Stubbed File-Mapping**: `SYS_MMAP` relies on a single global bump pointer confined to a 256 MB window (`0x60000000`–`0x70000000`), wraps blindly upon exhaustion, and ignores `fd` and `offset` for file-backed mapping.
4. **Missing Userland Unwind Foundation**: No ABI-compliant x86_64 `setjmp`/`longjmp` implementation exists in userspace, preventing runtime stack unwinding and exception handling.
5. **Incomplete Standard C/Math Runtime**: Functions required by C++ runtime operations (`math.h` intrinsics `sqrt`, `ceil`, `floor`, `fmod`, `fabs`, `strtol`, `errno`, `assert`) are absent from the userspace runtime.
6. **Lack of a Unified BOS Runtime Interface**: Userspace applications directly access raw syscall numbers rather than a standardized, stable ATOMS runtime boundary.

---

### 2. Forensic Evidence & Root-Cause Breakdown

#### 2.1 Thread-Local Storage (TLS) Invalidation
* **Evidence**:
  1. In [`kernel/core/scheduler/include/task.h`](file:///D:/Signatures_OS/kernel/core/scheduler/include/task.h#L31-L71), `struct Task` contains metadata for quantum, ticks, PML4, and stacks, but contains **no field** to store `fs_base`.
  2. In [`arch/x86_64/interrupt/isr_stubs.asm`](file:///D:/Signatures_OS/arch/x86_64/interrupt/isr_stubs.asm#L140-L157), lines 140 and 156 execute `mov fs, ax`. In x86_64 Long Mode, loading a 16-bit selector into `%fs` clears the hidden 64-bit base address to 0. Every hardware interrupt (e.g. IRQ0 timer) thus silently destroys any userspace TLS base.
  3. In [`kernel/core/scheduler/src/context_switch.asm`](file:///D:/Signatures_OS/kernel/core/scheduler/src/context_switch.asm#L36), `mov fs, ax` is likewise executed on every context restore.
  4. In [`atoms/userspace/runtime/tls/atoms_tls.c`](file:///D:/Signatures_OS/atoms/userspace/runtime/tls/atoms_tls.c#L18-L19), `s_thread_specific_values` is a single static global array shared across all threads, causing severe data races and lack of thread isolation.
  5. There is no syscall allowing Ring 3 to set or query its `%fs` base (`arch_prctl` or `SYS_SET_FS_BASE`).
* **Root Cause**: Architecture lacked MSR 0xC0000100 (`IA32_FS_BASE`) management in the scheduler and context-switch paths.

#### 2.2 Futex Synchronization Incompleteness
* **Evidence**:
  1. In [`kernel/core/syscall/src/services.c`](file:///D:/Signatures_OS/kernel/core/syscall/src/services.c#L646-L664):
     ```c
     int cmd = op & 0x7F;
     if (cmd == FUTEX_WAIT) {
         if (*uaddr != val) return (uint64_t)-1;
         scheduler_yield();
         return SYSCALL_OK;
     } else if (cmd == FUTEX_WAKE) {
         return val > 0 ? 1 : 0;
     }
     ```
  2. A thread calling `pthread_mutex_lock` or `pthread_cond_wait` yields once and immediately returns, spinning continuously in userspace while burning CPU cycles.
  3. No task is ever transitioned to `TASK_BLOCKED` or added to a wait queue.
  4. `FUTEX_WAKE` does not inspect or wake any sleeping threads.
* **Root Cause**: Futex subsystem was stubbed out as a placeholder without wiring to `scheduler_block_task()` and `scheduler_resume_task()`.

#### 2.3 Virtual Memory Allocation & Eager File Mapping
* **Evidence**:
  1. In [`kernel/core/syscall/src/services.c`](file:///D:/Signatures_OS/kernel/core/syscall/src/services.c#L528-L556), `s_user_mmap_bump` resets unconditionally to `0x60000000ULL` when reaching `0x70000000ULL`, causing overlapping allocations if prior memory has not been unmapped.
  2. Line 531 contains `(void)fd; (void)offset;`, ignoring file descriptors entirely.
  3. In [`kernel/core/syscall/src/services.c`](file:///D:/Signatures_OS/kernel/core/syscall/src/services.c#L620-L644), `sys_service_mprotect` only toggles `PAGE_WRITABLE` on existing PTEs and does not explicitly manage `VMM_FLAG_NX` or enforce W^X policy.
* **Root Cause**: Initial mmap was written as a minimal bump allocator for anonymous test pages rather than a general-purpose virtual memory manager.

#### 2.4 Missing Userland Unwind Primitives
* **Evidence**:
  1. Grep search confirms `setjmp` and `longjmp` are completely missing from [`userspace/runtime/c/`](file:///D:/Signatures_OS/userspace/runtime/c/).
  2. C++ runtime libraries and virtual machine interpreters requiring non-local gotos fail to link or compile.
* **Root Cause**: No x86_64 assembly implementation of the System V callee-saved register save/restore frame was created.

#### 2.5 C Runtime and Math Omissions
* **Evidence**:
  1. [`userspace/runtime/c/include/stdlib.h`](file:///D:/Signatures_OS/userspace/runtime/c/include/stdlib.h) declares `exit`, `abort`, `abs`, `labs`, `strtol`, but implementations are incomplete.
  2. No `math.h` exists in [`userspace/runtime/c/include/`](file:///D:/Signatures_OS/userspace/runtime/c/include/).
  3. No `assert.h` or `errno.h` exists.
* **Root Cause**: Userspace C runtime was incrementally expanded only for specific demo apps (Doom, Desktop Shell) without a formal libc baseline.

---

### 3. Files Involved

1. `kernel/core/scheduler/include/task.h` (Task definition, add `fs_base`)
2. `kernel/core/scheduler/src/scheduler.c` (Task switch MSR save/restore)
3. `kernel/core/scheduler/src/context_switch.asm` (Prevent `%fs` wipe, restore `fs_base`)
4. `arch/x86_64/interrupt/isr_stubs.asm` (Prevent `%fs` wipe during interrupt return)
5. `kernel/core/cpu/cpu_state.c` (CR4.FSGSBASE enable if supported)
6. `kernel/core/syscall/include/syscall.h` (Add `SYS_SET_FS_BASE`, `SYS_GET_FS_BASE`)
7. `kernel/core/syscall/src/dispatcher.c` (Dispatch new syscalls)
8. `kernel/core/syscall/src/services.c` (Implement FS base, Futex wait/wake, mmap file-backing, mprotect W^X)
9. `userspace/runtime/c/src/setjmp.asm` (New x86_64 System V setjmp/longjmp)
10. `userspace/runtime/c/include/setjmp.h` (New header)
11. `userspace/runtime/c/include/math.h` & `userspace/runtime/c/src/math.c` (New math routines)
12. `userspace/runtime/c/include/assert.h` & `userspace/runtime/c/include/errno.h` (New headers)
13. `userspace/runtime/c/src/stdlib.c` (Implement missing stdlib functions)
14. `userspace/runtime/c/src/pthread.c` (Update pthread TLS to use FS base)
15. `userspace/runtime/include/atoms_runtime.h` (New unified runtime API)
16. `userspace/tests/phase1_runtime_test/main.cpp` (New comprehensive validation harness)

---

### 4. Risk Analysis

* **Task Switching / Scheduler Invalidation**: Changing register handling in `isr_stubs.asm` or `context_switch.asm` can crash the kernel or cause desktop freeze if segment selectors are loaded improperly.
* **Futex Deadlock**: Flaws in the futex wait/wake queue could cause threads to remain permanently in `TASK_BLOCKED` state.
* **Memory Corruption**: Expanding `mmap` could overwrite active process memory if user boundary checks fail.
* **Syscall ABI Breakage**: Adding new syscalls must strictly append to the syscall table without modifying existing syscall IDs (0–43).

---

### 5. Suspected Fix Strategy (NO CODE)

1. Append `fs_base` to `struct Task`. Read MSR 0xC0000100 on switch-out, write MSR 0xC0000100 on switch-in. Remove unconditional `%fs` reset from interrupt/context switch paths.
2. Implement `SYS_SET_FS_BASE` and `SYS_GET_FS_BASE` with strict canonical address validation (`USER_WINDOW_MIN` to `USER_WINDOW_MAX`).
3. Construct a kernel-side Futex Waiter Table in `services.c` linking blocked tasks via `scheduler_block_task()` and resuming them via `scheduler_resume_task()`.
4. Implement eager file reading in `sys_service_mmap` when `fd >= 0` and `!(flags & MAP_ANONYMOUS)`.
5. Write standalone `setjmp.asm` saving `rbx, rbp, r12, r13, r14, r15, rsp, rip`.
6. Implement missing C and math library primitives.
7. Wrap runtime operations in a clean `atoms_runtime.h` abstraction.
8. Build and verify under QEMU UEFI boot with zero regressions.
