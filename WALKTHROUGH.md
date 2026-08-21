# WALKTHROUGH — Syscall Return Frame & Nested IRQ Context Isolation

## 1. Problem Addressed
During high-frequency GUI polling (`SYS_GUI_POLL_EVENT`), Timer IRQ 0 preempted `desktop_shell` while it was executing inside CPL 0 (kernel mode). Because the kernel stack was being used for both nested interrupt frames and the syscall return frame, the saved user RIP could become contaminated with an address from the task's kernel stack (`0x11035F70`), leading to `#PF(5)` on `sysret` when Ring 3 attempted to execute code from supervisor memory.

---

## 2. Changes Implemented

### 1. Task-Isolated Immutable Syscall Return Context
- In [`kernel/core/scheduler/include/task.h`](file:///d:/Signatures_OS/kernel/core/scheduler/include/task.h):
  Added dedicated fields `syscall_user_rip`, `syscall_user_rsp`, `syscall_user_rflags` to `struct Task`.
- In [`kernel/core/syscall/src/syscall.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall.c):
  - `syscall_handler` snapshots `frame->user_rip`, `frame->user_rsp`, and `frame->user_rflags` directly into the `current` task structure upon entry.
  - `syscall_prepare_return` verifies that if the stack frame was altered or corrupted by nested calls or IRQ preemption, the valid userspace return RIP/RSP is automatically recovered from the immutable task context.

### 2. Segment Register Protection & Canonical `iretq` Return
- In [`kernel/core/syscall/src/syscall_entry.asm`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall_entry.asm):
  - Segment registers `DS` and `ES` are explicitly reloaded with usermode data selector `0x1B` prior to returning to Ring 3.
  - Returns are processed through canonical `iretq` (`RETURN_IRET`), guaranteeing atomic CPU state transitions across preemption boundaries.

---

## 3. Build & Test Status
- Clean build: **Exit code 0** via `build.ps1`.
- Images updated: `build/OS.img` and `build/SignaturesOS.vmdk`.
