# 🔬 FORENSIC INVESTIGATION REPORT: BOOT SEQUENCE INPUT BRING-UP & CURSOR VISIBILITY ISOLATION
**Subsystem:** ATOMS OS Input & Boot Subsystem (`kernel.c`, `kernel_input_init`, `ps2_mouse`, `vmmouse`, `ROOK`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-15  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Root Cause Analysis
1. **Boot Sequence Ordering Bug:** `kernel_input_init()`, `ps2_mouse_init()`, and `vmmouse_init()` were only scheduled in a post-login test task (`atoms_cursor_certification_init()`), leaving `InputCore` with zero registered consumers and hardware mouse controllers uninitialized during `ROOK_PAGE_LOGIN`.
2. **Cursor Visibility Lifecycle:** The mouse cursor should not be drawn during Ring 0 Boot Splash or Dashboard, but drivers must be initialized silently in the background so the pointer is immediately active upon reaching the Login Screen.

---

## 2. Real OS Parity (macOS BootX64 / Windows NT `ntoskrnl` Standard)
* Windows NT / macOS bootstrap the mouse class drivers silently during boot animation without blitting the pointer glyph.
* Once the logon session manager activates (`LogonUI` / `loginwindow`), the compositor enables pointer rendering.

---

## 3. Files Involved
1. `kernel/kernel.c`: Call `kernel_input_init()`, `kernel_input_update_resolution()`, `ps2_mouse_init()`, and `vmmouse_init()` before `rook_init()`.
2. `kernel/shell/rook/src/rook_render.c`: Condition cursor blit on `current->id == ROOK_PAGE_LOGIN`.
