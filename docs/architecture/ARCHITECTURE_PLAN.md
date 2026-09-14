# ATOMS OS — Architecture Plan: Kernel Reliability, Context Safety & Triple Fault Elimination

## 1. Architectural Objectives

1. **Eliminate Virtual CPU Shutdown / Triple Faults**:
   - Install dedicated TSS IST (Interrupt Stack Table) for Double Fault (`#DF`, Vector 8) so stack corruption can never escalate to CPU shutdown.
   - Prevent `idle_task` runqueue starvation when the shell faults.
2. **Canonical RFLAGS Sanitization on All Ring 3 Return Paths**:
   - Mask out Nested Task (`NT`), Resume Flag (`RF`), and `IOPL` from user RFLAGS on every `iretq` or `sysret` return.
3. **1080p Wallpaper 1:1 Fast-Path Engine**:
   - Implement zero-division 64-bit row streaming in `sys_service_gui_draw_wallpaper()` when source and destination dimensions match ($1920 \times 1080$), reducing rendering latency from ~80ms to <0.5ms.
4. **Shell Guardian & Crash Dump Engine**:
   - Capture complete register frame (RIP, RSP, CR2, CS, SS, RFLAGS, faulting vector) on COM1 when any user-space fault occurs and safely restart the desktop shell.

---

## 2. File Change Plan

| File | Target Function | Architectural Change |
|---|---|---|
| [`kernel/core/syscall/src/syscall.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall.c) | `syscall_prepare_return()` | Enforce `(frame->user_rflags & 0x00000CD5ULL) \| 0x00000202ULL`. |
| [`kernel/core/syscall/src/services.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c) | `sys_service_gui_draw_wallpaper()` | Add 1:1 fast path with direct `memcpy()` per scanline when `src_w == dest_w && src_h == dest_h`. |
| [`kernel/core/interrupt/src/exception.c`](file:///d:/Signatures_OS/kernel/core/interrupt/src/exception.c) | `exception_dispatch()` | Complete forensic telemetry logging and safe process fault containment. |

---

## 3. Rollback Strategy
- Every component retains strict binary compatibility with existing ABI and System V calling conventions. If any change shows regression in QEMU pre-flight, it can be reverted cleanly via git.
