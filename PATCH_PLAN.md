# 📐 ARCHITECTURE PATCH PLAN: BOOT SEQUENCE INPUT BRING-UP & CURSOR VISIBILITY ISOLATION
**Subsystem:** ATOMS OS Input & Boot Subsystem (`kernel.c`, `kernel_input_init`, `ps2_mouse`, `vmmouse`, `ROOK`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-15  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Integrate silent background input driver bring-up (`kernel_input_init()`, `ps2_mouse_init()`, `vmmouse_init()`) during early boot before ROOK splash.
- Enforce cursor visibility isolation: 0% visibility on Boot Splash and Dashboard, 100% active visibility on Login Screen.

---

## 2. Target Files for Modification
1. `kernel/kernel.c`:
   - Bring up `kernel_input_init()`, `kernel_input_update_resolution()`, `ps2_mouse_init()`, and `vmmouse_init()` before `rook_init()`.
2. `kernel/shell/rook/src/rook_render.c`:
   - Wrap `arya_compositor_draw_cursor()` in `if (current && current->id == ROOK_PAGE_LOGIN)` guard.

---

## 3. Expected Engineering Results
- **Boot Splash / Dashboard:** Pure black canvas, clean rotating spinner, no mouse cursor drawn.
- **Login Screen:** Mouse cursor immediately responsive to USB mouse (real H81 hardware), VMware VMMouse, and PS/2 mouse with zero latency.

---

## 4. Rollback Plan
Revert changes to Git commit `29f1549`.
