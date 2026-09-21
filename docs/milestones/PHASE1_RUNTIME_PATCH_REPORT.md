# ATOMS OS — Phase 1 BOS Userland Runtime Foundation Patch Report
**Document ID:** `PHASE1_RUNTIME_PATCH_REPORT.md`  
**Milestone:** Java Runtime Phase 1 — BOS Ring 3 Userland Runtime Foundation  
**Protocol:** ATOMS OS Engineering Protocol V1 (Task 3 Output)  
**Date:** September 14, 2026  
**Author:** Antigravity / Saumya Chaudhari  
**Target Platform:** Haswell x86_64 LGA1150 / Pure UEFI / QEMU x86_64  

---

## 1. Executive Summary

In accordance with Rule 0 of the ATOMS OS Development Protocol, this report details the concrete source code changes executed to complete **Phase 1: BOS Userland Runtime Foundation**. The objective was to prepare and solidify the native BOS Ring 3 runtime environment for subsequent embedded JVM (Avian) integration without vendoring the JVM prematurely.

All patches strictly adhered to [`docs/milestones/PHASE1_RUNTIME_PATCH_PLAN.md`](file:///D:/Signatures_OS/docs/milestones/PHASE1_RUNTIME_PATCH_PLAN.md).

---

## 2. Modified Files Ledger

| # | File Path | Subsystem | Nature of Change |
|---|-----------|-----------|------------------|
| 1 | `kernel/core/scheduler/include/task.h` | Kernel Scheduler | Added `uint64_t fs_base;` to `struct Task`. |
| 2 | `arch/x86_64/interrupt/isr_stubs.asm` | Architecture Assembly | Removed `mov fs, ax` / `mov gs, ax` from usermode segment reloads to prevent clobbering `IA32_FS_BASE`. |
| 3 | `kernel/core/scheduler/src/context_switch.asm` | Architecture Assembly | Removed `mov fs, ax` / `mov gs, ax` from task switch path to preserve `IA32_FS_BASE`. |
| 4 | `kernel/core/cpu/cpu_state.c` | CPU Initialization | Enabled `CR4.FSGSBASE` (bit 16) if supported by CPUID standard feature leaf (7.EBX[0]). |
| 5 | `kernel/core/scheduler/src/scheduler.c` | Kernel Scheduler | Added MSR read/write helpers; wired saving and restoring of `IA32_FS_BASE` MSR (0xC0000100) on task switch. |
| 6 | `kernel/core/syscall/include/syscall.h` | Syscall Interface | Defined `SYS_SET_FS_BASE` (44U) and `SYS_GET_FS_BASE` (45U); updated `MAX_SYSCALL` to 46U. |
| 7 | `kernel/core/syscall/src/dispatcher.c` | Syscall Dispatcher | Added dispatch cases for `SYS_SET_FS_BASE` and `SYS_GET_FS_BASE`. |
| 8 | `kernel/core/syscall/src/services.c` | Kernel Syscall Services | Implemented `sys_service_set_fs_base` and `sys_service_get_fs_base`; implemented real 64-slot `FutexWaiter` wait/wake table; added eager file-backed `mmap` from VFS (`vfs_pread`); wired `PAGE_NX` in `mprotect`. |
| 9 | `userspace/runtime/c/include/atoms_syscall.h` | Userspace Syscall API | Defined `SYS_SET_FS_BASE 44U` and `SYS_GET_FS_BASE 45U`. |
| 10 | `userspace/runtime/c/include/pthread.h` | Userspace Pthread Header | Added `pthread_key_t`, `pthread_key_create`, `pthread_key_delete`, `pthread_setspecific`, `pthread_getspecific`, `atoms_set_fs_base`, `atoms_get_fs_base`. |
| 11 | `userspace/runtime/c/include/stdlib.h` | Standard C Header | Added `__attribute__((noreturn))` to `exit` and `abort`. |
| 12 | `userspace/runtime/c/src/atoms_syscall.c` | Userspace Syscall Bridge | Removed duplicate `exit` and `abort` implementations. |
| 13 | `userspace/runtime/c/src/pthread.c` | Userspace Pthread Lib | Implemented true per-thread TCB allocated via `atoms_set_fs_base` and thread trampoline for isolated TLS. |
| 14 | `atoms/userspace/runtime/tls/atoms_tls.c` | APAL/Musl TLS Adapter | Replaced global static storage array with per-thread TCB using `SYS_SET_FS_BASE`. |
| 15 | `userspace/runtime/c/src/crt0.asm` | NASM C Runtime Bootstrap | Added `call __libc_init_array` prior to `call main`. |
| 16 | `userspace/runtime/c/src/crt0.S` | GAS C Runtime Bootstrap | Added `call __libc_init_array` prior to `call main`. |
| 17 | `userspace/linker.ld` | Userspace Linker Script | Added `.init_array` and `.fini_array` sections for C++ global constructors and destructors. |
| 18 | `BUILD.gn` | Root GN Build Config | Added `stdlib.c`, `math.c`, `setjmp.S` to `atoms_runtime_c` target. |
| 19 | `build.ps1` | Master Build Script | Added compilation of `stdlib.c`, `math.c`, `setjmp.asm`, `user_crt0.o`, and `user_phase1_runtime_test.o`. |
| 20 | `userspace/tests/toolchain_test/atoms_c_test.c` | Toolchain C Verification | Enhanced to test Math IEEE 754, Setjmp/Longjmp, Mmap dynamic pages, and TLS FS Base. |

---

## 3. Created Files Ledger

| # | File Path | Subsystem | Purpose |
|---|-----------|-----------|---------|
| 1 | `userspace/runtime/c/include/setjmp.h` | C Runtime | System V x86_64 ABI `jmp_buf`, `setjmp`, `longjmp` declarations. |
| 2 | `userspace/runtime/c/src/setjmp.asm` | C Runtime | NASM implementation of callee-saved register save/restore for `setjmp`/`longjmp`. |
| 3 | `userspace/runtime/c/src/setjmp.S` | C Runtime | GNU AS implementation of `setjmp`/`longjmp` for GN/Ninja toolchain. |
| 4 | `userspace/runtime/c/include/errno.h` | C Runtime | Standard POSIX error codes and thread-safe `__errno_location` / `errno` macro. |
| 5 | `userspace/runtime/c/include/assert.h` | C Runtime | Standard `assert()` macro and `__assert_fail()` runtime handler. |
| 6 | `userspace/runtime/c/include/math.h` | C Runtime | Freestanding IEEE 754 declarations (`fabs`, `sqrt`, `floor`, `ceil`, `fmod`, `trunc`, `round`, `isnan`, `isinf`). |
| 7 | `userspace/runtime/c/src/math.c` | C Runtime | Freestanding bitwise and Newton-Raphson implementation of IEEE 754 math functions. |
| 8 | `userspace/runtime/c/src/stdlib.c` | C Runtime | Implementations of `exit`, `abort`, `abs`, `labs`, `strtol`, `__assert_fail`, `__errno_location`, `__libc_init_array`, `__libc_fini_array`. |
| 9 | `userspace/runtime/include/atoms_runtime.h` | Unified API | Master BOS Ring 3 canonical runtime header aggregating syscalls, memory, threading, TLS, math, and unwinding. |
| 10 | `userspace/tests/phase1_runtime_test/phase1_runtime_test.h` | Verification | Header for Phase 1 verification suite and report structure. |
| 11 | `userspace/tests/phase1_runtime_test/phase1_runtime_test.cpp` | Verification | Comprehensive 10-test deterministic hardware suite exercising all Phase 1 capabilities. |
| 12 | `tools/test_phase1_qemu.py` | Tooling | Automated QEMU pure UEFI pre-flight test runner with interactive login and screendump capture. |

---

## 4. Verification & Build Impact

- Zero build warnings/errors in the kernel (`kernel.bin`).
- Zero build warnings/errors in bootloader (`BOOTX64.EFI`).
- Master GN+Ninja targets (`atoms_c_test.elf`, `atoms_cpp_test.elf`, `v8_test_runner.elf`, `minimal_real_browser.elf`) generated cleanly.
- Master storage artifacts (`build/OS.img`, `build/SignaturesOS.vdi`, `build/SignaturesOS.vmdk`, `build/atoms_uefi_test.img`) regenerated cleanly.
