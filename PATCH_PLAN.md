# ATOMS OS — Patch Plan: Fast Mouse Event Loop & Syscall Frame Isolation

## 1. Objectives

1. Eliminate stack overlap risk in `desktop_shell` by moving `BOS_GUIEvent` out of the call stack into static memory.
2. Refine `syscall_prepare_return()` to trust the validated per-frame context on the task's private kernel stack.

---

## 2. File Change Plan

| File | Target Function | Architectural Change |
|---|---|---|
| [`userspace/apps/desktop_shell/main.c`](file:///d:/Signatures_OS/userspace/apps/desktop_shell/main.c) | `main()` | Change `BOS_GUIEvent event;` to `static BOS_GUIEvent event;` to guarantee zero stack perturbation. |
| [`kernel/core/syscall/src/syscall.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall.c) | `syscall_prepare_return()` | Use stack frame context primarily; fallback to task cached fields only when frame context is invalid. |

---

## 3. Rollback Plan
- Revert changes to `desktop_shell/main.c` and `syscall.c` if needed.
