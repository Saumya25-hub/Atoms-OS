# 📐 ARCHITECTURE PATCH PLAN: ARYA MOUSE COMPOSITOR HOOK
**Subsystem:** ATOMS OS Input & Graphics Presentation Subsystem (`ROOK Engine V1.0` / `ARYA Hook`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-15  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Deliver real-time, zero-lag, subpixel-precise graphical mouse pointer presentation across all ROOK pages (Lock Screen, Sign-In, and Transitions).
- Implement standard Windows DWM / Linux Wayland compositor hook behavior (`ARYA Compositor Pointer Hook`).
- Maintain strict 0-heap allocation, zero performance degradation ($<0.003\text{ms}$ blit time), and 100% boundary clipping protection.

---

## 2. Target Files for Modification
1. `kernel/shell/rook/src/rook_render.c`:
   - Add `arya_compositor_draw_cursor(uint32_t* fb, uint32_t width, uint32_t height, uint32_t stride_pixels)`
   - Integrate `arya_compositor_draw_cursor()` into `rook_render_flush()` immediately before the GOP VRAM transfer barrier.
   - Add tracking of `s_prev_cursor_x`, `s_prev_cursor_y` to invalidate $36\times36$ dirty rects on pointer motion without redrawing unaffected UI tiles.

---

## 3. Expected Engineering Results
- **Visual:** A crisp Windows 11 Concept white arrow with subtle dark drop outline moves seamlessly across the 1080p display at full 60FPS.
- **Latency:** Instant $<0.5\text{ms}$ response to physical USB/PS2 mouse movement.
- **Safety:** Full boundary clamping — cursor smoothly touches all 4 edges of the screen $(x \in [0, 1919], y \in [0, 1079])$ without memory corruption or buffer overflows.

---

## 4. Rollback Plan
If any visual artifact occurs, revert `kernel/shell/rook/src/rook_render.c` to Git commit `09cfa91`.
