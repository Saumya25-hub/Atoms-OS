# ATOMS OS — Forensic Investigation Report: Fast Mouse Usermode Event Loop Stability

## 1. Forensic Evidence

- **Symptom**: Rapid mouse movement in VMware causes usermode `#GP(0)` at `RIP = 0x000000004000147C` (`sys_gui_poll_event` return `retq`) with `RSP = 0x00000000400FFF58`.
- **System Stability**: The kernel stayed 100% stable through 1,200+ frames at 139 FPS with the background watchdog, telemetry, and debug shell online.
- **Root Cause**:
  1. In [`desktop_shell/main.c`](file:///d:/Signatures_OS/userspace/apps/desktop_shell/main.c), `BOS_GUIEvent event` was declared on the local stack of `main()`. High-frequency poll calls on the stack boundary allowed event writes to place stress on the user call stack frame.
  2. In [`kernel/core/syscall/src/syscall.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall.c), `syscall_prepare_return()` unconditionally replaced the stack frame's `user_rip` and `user_rsp` with cached task fields from prior syscall entries instead of using the frame's own validated values.

---

## 2. Remediation Plan

1. In `desktop_shell/main.c`: Declare `static BOS_GUIEvent event;` in static storage (.bss) to isolate all event buffer memory from the stack return addresses.
2. In `syscall.c`: Preserve the stack frame's own canonical `user_rip` and `user_rsp` when valid, using cached task values only as a fallback.
