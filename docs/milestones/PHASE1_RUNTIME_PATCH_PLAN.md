# ATOMS OS — PHASE 1 BOS USERLAND RUNTIME FOUNDATION
## TASK 2: ARCHITECTURAL PATCH PLAN

**Input Document**: `docs/milestones/PHASE1_RUNTIME_FORENSIC_REPORT.md`  
**Protocol Phase**: TASK 2 (Architecture & Patch Plan — Zero Source Code Modification)  
**Date**: September 14, 2026  
**Architect**: ATOMS Architecture Team  

---

### 1. Architectural Scope & Objectives

The goal of Phase 1 is to convert the BOS userspace runtime into a robust, isolated execution host capable of running a multi-threaded C++ runtime and embedded Java Virtual Machine without requiring Linux compatibility, POSIX emulation bloat, or sacrificing BOS Kernel stability.

---

### 2. Surgical File Modification Ledger

The following files, and ONLY these files, are approved for modification or creation in Task 3:

| File Path | Action | Architectural Purpose |
| :--- | :--- | :--- |
| `kernel/core/scheduler/include/task.h` | Modify | Add `uint64_t fs_base;` field to `struct Task`. |
| `kernel/core/scheduler/src/scheduler.c` | Modify | Save `IA32_FS_BASE` on task switch-out and restore on task switch-in for user tasks. |
| `arch/x86_64/interrupt/isr_stubs.asm` | Modify | Remove `mov fs, ax` segment wipe from interrupt return path; preserve `%fs` base. |
| `kernel/core/scheduler/src/context_switch.asm` | Modify | Remove `mov fs, ax` segment wipe; ensure `%fs` selector is maintained without wiping base. |
| `kernel/core/cpu/cpu_state.c` | Modify | Enable CR4.FSGSBASE (bit 16) during CPU initialization when supported by CPUID. |
| `kernel/core/syscall/include/syscall.h` | Modify | Define `SYS_SET_FS_BASE` (44U), `SYS_GET_FS_BASE` (45U), update `MAX_SYSCALL` to 46U. |
| `kernel/core/syscall/src/dispatcher.c` | Modify | Dispatch `SYS_SET_FS_BASE` and `SYS_GET_FS_BASE` in the syscall switch statement. |
| `kernel/core/syscall/src/services.c` | Modify | Implement `sys_service_set_fs_base`, `sys_service_get_fs_base`, real Futex wait/wake queue, eager file-backed mmap, and W^X NX check in `sys_service_mprotect`. |
| `userspace/runtime/c/include/setjmp.h` | Create | Standard x86_64 `jmp_buf` definition and prototypes for `setjmp`/`longjmp`. |
| `userspace/runtime/c/src/setjmp.asm` | Create | System V ABI implementation of `setjmp` and `longjmp`. |
| `userspace/runtime/c/include/errno.h` | Create | Standard POSIX-compatible error numbers (`EAGAIN`, `EINVAL`, `ENOMEM`, `EFAULT`). |
| `userspace/runtime/c/include/assert.h` | Create | Standard `assert(expr)` macro wired to console logging and abort. |
| `userspace/runtime/c/include/math.h` | Create | Prototypes for `sqrt`, `floor`, `ceil`, `fmod`, `fabs`. |
| `userspace/runtime/c/src/math.c` | Create | Hardware-accelerated math routines using SSE / x87 instructions. |
| `userspace/runtime/c/src/stdlib.c` | Create | Implement `exit`, `abort`, `abs`, `labs`, `strtol`. |
| `userspace/runtime/c/src/pthread.c` | Modify | Update TLS helpers (`pthread_setspecific`, `pthread_getspecific`) to utilize `%fs` base. |
| `userspace/runtime/include/atoms_runtime.h` | Create | Canonical unified BOS userland runtime API header. |
| `userspace/tests/phase1_runtime_test/main.cpp` | Create | Comprehensive Phase 1 verification test suite. |
| `build.ps1` | Modify | Add compilation and linking for `setjmp.asm`, `math.c`, `stdlib.c`, and the Phase 1 test suite. |

---

### 3. Detailed Modification Plan per Component

#### 3.1 Subsystem 1: Thread-Local Storage (TLS) Architecture
* **What to Modify**:
  - In `task.h`: Insert `uint64_t fs_base;` right after `uint64_t syscall_user_rflags;`.
  - In `scheduler.c`: In `scheduler_on_tick()` around line 837 (prior to context switch), if `old_task && old_task->is_user_task`, read MSR 0xC0000100 into `old_task->fs_base`. If `new_task && new_task->is_user_task`, write `new_task->fs_base` into MSR 0xC0000100.
  - In `isr_stubs.asm` and `context_switch.asm`: Remove `mov fs, ax` so that the CPU's hidden base register is never cleared on interrupt or context return.
  - In `cpu_state.c`: During `ensure_sse_control_registers()`, check if CPUID (EAX=7, ECX=0, EBX bit 0) indicates FSGSBASE support; if present, enable CR4 bit 16.
  - In `syscall.h`: Define `SYS_SET_FS_BASE (44U)` and `SYS_GET_FS_BASE (45U)`.
  - In `services.c`: Implement `sys_service_set_fs_base(uint64_t base)`. Validate that `base >= USER_WINDOW_MIN && base < USER_WINDOW_MAX`. Update `current->fs_base = base;` and call `write_msr(0xC0000100, base)`.
* **Expected Result**: Thread A and Thread B can set completely independent TLS bases that survive timer interrupts and context switches without cross-talk or corruption.
* **Risk**: Low-to-medium. Validating `base` ensures Ring 3 cannot set kernel-space TLS addresses.

#### 3.2 Subsystem 2: Futex & Real Synchronization Architecture
* **What to Modify**:
  - In `services.c`: Define a fixed kernel-level table `FutexWaiter s_futex_waiters[64]`, protected by an atomic spinlock or interrupt save/restore.
  - On `FUTEX_WAIT`: Validate user pointer. Verify `*uaddr == val`. If unequal, return `-1` (`EAGAIN`). Otherwise, record `(uaddr, current_task)` in the table, call `scheduler_block_task(current_task)`, and execute a sleep loop (`while (current_task->state == TASK_BLOCKED) __asm__ volatile("sti; hlt" ::: "memory");`). Clear the slot upon waking and return `SYSCALL_OK`.
  - On `FUTEX_WAKE`: Validate user pointer. Iterate through `s_futex_waiters`, find up to `val` entries matching `uaddr`, remove them from the table, and invoke `scheduler_resume_task(waiter->task)`. Return the number of woken threads.
* **Expected Result**: Eliminates busy-waiting. Contending threads enter low-power sleep state and are awakened deterministically.
* **Risk**: Low. Max 64 concurrent waiters is plenty for userland tests; bounds checking prevents table overflows.

#### 3.3 Subsystem 3: Virtual Memory Allocation & Eager File-Backed Mapping
* **What to Modify**:
  - In `services.c`:
    1. Update `sys_service_mmap`: When `addr == 0 || !(flags & MAP_FIXED)`, search for an available contiguous page range starting from `0x48000000ULL` up to `0x78000000ULL` (allowing up to 768 MB of user dynamic virtual memory).
    2. Add file-backed mapping support: If `fd >= 0 && !(flags & MAP_ANONYMOUS)`, validate `fd`. Seek to `offset` using `vfs_seek`, and for each newly allocated page frame, read up to 4096 bytes from `fd` into the physical buffer with `vfs_read()` before mapping.
    3. Update `sys_service_mprotect`: Check for W^X violation (`(prot & PROT_WRITE) && (prot & PROT_EXEC)`). If violated, return `SYSCALL_FAIL`. If `prot & PROT_EXEC`, clear `VMM_FLAG_NX`; otherwise set `VMM_FLAG_NX`.
* **Expected Result**: Robust multi-allocation `mmap` with support for eager file loading (enabling future `.class` and `.jar` mapping) and strict W^X enforcement.
* **Risk**: Low. Fully verified against isolated PML4 tables.

#### 3.4 Subsystem 4: Userland Unwind Foundation (`setjmp` / `longjmp`)
* **What to Modify**:
  - Create `userspace/runtime/c/src/setjmp.asm`: Implement `setjmp` saving `rbx, rbp, r12, r13, r14, r15, rsp, rip` and returning 0. Implement `longjmp` restoring registers and returning `val ? val : 1`.
  - Create `userspace/runtime/c/include/setjmp.h`: Typedef `uint64_t jmp_buf[8];`.
* **Expected Result**: Fully compliant System V AMD64 non-local jump mechanism for C/C++ runtimes.
* **Risk**: Negligible. Standard leaf assembly routines.

#### 3.5 Subsystem 5: Standard C & Mathematics Runtime
* **What to Modify**:
  - Create `userspace/runtime/c/include/math.h` and `src/math.c`:
    - `sqrt(double x)`: Emits `sqrtsd`.
    - `floor(double x)`: Emits `roundsd $1`.
    - `ceil(double x)`: Emits `roundsd $2`.
    - `fabs(double x)`: Clears sign bit via mask or `sqrtsd(x * x)`.
    - `fmod(double x, double y)`: Uses x87 `fprem` or standard range reduction.
  - Create `userspace/runtime/c/src/stdlib.c`: Implement `exit(status)` (calls `__atoms_syscall1(SYS_EXIT, status)`), `abort()` (logs crash and calls `exit(134)`), `abs(j)`, `labs(j)`, and `strtol()`.
  - Create `userspace/runtime/c/include/errno.h` and `assert.h`.
* **Expected Result**: Zero unresolved symbols for standard math and library calls in userspace applications.
* **Risk**: Negligible. Pure userland functions.

#### 3.6 Subsystem 6: Unified ATOMS Runtime Boundary (`atoms_runtime.h`)
* **What to Modify**:
  - Create `userspace/runtime/include/atoms_runtime.h`: Clean, modern API encapsulating:
    - `atoms_mem_alloc(size, prot, flags)`
    - `atoms_mem_free(addr, size)`
    - `atoms_mem_protect(addr, size, prot)`
    - `atoms_mem_map_file(fd, offset, size, prot)`
    - `atoms_thread_spawn(entry, stack_top, arg)`
    - `atoms_thread_exit(code)`
    - `atoms_tls_set(ptr)`
    - `atoms_tls_get()`
    - `atoms_sync_futex_wait(addr, val)`
    - `atoms_sync_futex_wake(addr, count)`
    - `atoms_time_now_ms()`
    - `atoms_time_sleep_ms(ms)`
    - `atoms_file_open(path, flags, mode)`
    - `atoms_file_read(fd, buf, count)`
    - `atoms_file_write(fd, buf, count)`
    - `atoms_file_seek(fd, offset, whence)`
    - `atoms_file_close(fd)`
* **Expected Result**: Decouples the future JVM from raw kernel internals.

---

### 4. Rollback Strategy

If any regression occurs during build or QEMU pre-flight validation:
1. Revert changes via Git checkout of the affected file.
2. Re-verify clean baseline compilation via `build.ps1`.
3. Isolate the specific subsystem causing failure before re-attempting.
