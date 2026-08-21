# ATOMS OS — Patch Report: Event Structure Stack Isolation & Per-Syscall Frame Validation

## 1. Files & Functions Modified

| File | Target Function | Changes Applied |
|---|---|---|
| [`userspace/apps/desktop_shell/main.c`](file:///d:/Signatures_OS/userspace/apps/desktop_shell/main.c) | `main()` | Declared `static BOS_GUIEvent event;` in `.bss` memory to ensure that high-frequency mouse event writes during fast movement cannot touch or stress the user call stack frame and return addresses. |
| [`kernel/core/syscall/src/syscall.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall.c) | `syscall_prepare_return()` | Refined validation logic so the hardware/stack frame's own `user_rip` and `user_rsp` are preserved when canonical and valid, using cached task values only as a recovery fallback. |

---

## 2. Compilation Results
- **Status**: `PASS` (zero errors)
- **Target Artifacts**:
  - `build/SignaturesOS.vmdk`
  - `build/SignaturesOS.vmx`
  - `build/OS.img`
