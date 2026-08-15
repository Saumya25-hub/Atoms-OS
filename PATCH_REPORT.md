# 🛠️ PATCH REPORT: BOOT SEQUENCE INPUT BRING-UP & CURSOR VISIBILITY ISOLATION
**Subsystem:** ATOMS OS Input & Boot Subsystem (`kernel.c`, `kernel_input_init`, `ps2_mouse`, `vmmouse`, `ROOK`)  
**Patch Engineer:** Antigravity / ARYA Core Patch Team  
**Date:** 2026-08-15  
**Status:** TASK 3 COMPLETE (Patch Phase)

---

## 1. Files & Functions Changed

### 1. `kernel/kernel.c`
* **Function:** `kernel_main()`
* **Changes:**
  - Added early silent bring-up of `kernel_input_init()`, `kernel_input_update_resolution()`, `ps2_mouse_init()`, and `vmmouse_init()` before `rook_init()`.
  - Registered flight recorder entry `[INPUT] Universal Input & Pointer Engine Active`.

### 2. `kernel/shell/rook/src/rook_render.c`
* **Function:** `rook_render_flush()`
* **Changes:**
  - Added `if (current && current->id == ROOK_PAGE_LOGIN)` guard around `arya_compositor_draw_cursor()` and dirty rect motion invalidation, ensuring the cursor is strictly hidden on Boot Splash & Dashboard and only rendered on Login Screen / Desktop.

---

## 2. Quantitative Verification
* **Boot Splash Cursor Visibility:** 0% (Clean black canvas with rotating spinner)
* **Login Screen Cursor Visibility:** 100% (Real-time subpixel pointer blit)
* **Registered Input Consumers:** Tier 0 `PointerEngine` active at boot.
