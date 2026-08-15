# 🔬 FORENSIC INVESTIGATION REPORT: V1 CORE GUI SYSCALL ABI & USER-SPACE WINDOW SERVER ARCHITECTURE
**Subsystem:** ATOMS OS Kernel Syscall Gateway & Userspace Graphics Interface (`syscall.h`, `dispatcher.c`, `services.c`, `validation.c`, `syscalls_gui.h`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-16  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE MODIFIED)

---

## 1. Executive Summary & Architectural Audit
To migrate the ATOMS OS Desktop Shell from Ring 0 to Ring 3 cleanly without compromising system stability, the Kernel must act strictly as a **Protected Window Server & Compositor**, while the User Mode Desktop processes render into private window surfaces.

### Key Architectural Invariants:
1. **Strict Window Primitive Isolation:** The kernel syscall interface shall provide ONLY low-level window surface primitives (Create, Destroy, Show/Hide, Set Bounds, Map Surface, Invalidate, Poll Event, Screen Info).
2. **Widgets in Userspace:** Buttons, labels, panels, textboxes, and visual styling shall NOT live in kernel syscalls; they belong entirely in `libbos_gui` userspace software rendering.
3. **Zero Direct Framebuffer Exposure:** User processes never see physical VRAM or global compositor memory addresses.
4. **Strict Security Validation:** Every user pointer and window ID must be validated against `[USER_WINDOW_MIN, USER_WINDOW_MAX)` and process ownership (`win->owner_pid == current_process->pid`).

---

## 2. V1 Core GUI Syscall Range (16 - 23)
* `16`: `SYS_GUI_CREATE_WINDOW`
* `17`: `SYS_GUI_DESTROY_WINDOW`
* `18`: `SYS_GUI_SHOW_WINDOW`
* `19`: `SYS_GUI_SET_BOUNDS`
* `20`: `SYS_GUI_MAP_SURFACE`
* `21`: `SYS_GUI_INVALIDATE`
* `22`: `SYS_GUI_POLL_EVENT`
* `23`: `SYS_GUI_GET_SCREEN_INFO`

---

## 3. Files Involved
* `kernel/core/syscall/include/syscall.h`: Add syscall constants & GUI types.
* `kernel/core/syscall/src/validation.c`: Add user pointer validation routines.
* `kernel/core/syscall/src/services.c`: Implement kernel-side window surface and event handling services.
* `kernel/core/syscall/src/dispatcher.c`: Wire syscall IDs 16-23 into dispatcher switch table.
* `userspace/libbos_gui/include/syscalls_gui.h`: Mirror exact frozen ABI in userspace header.
