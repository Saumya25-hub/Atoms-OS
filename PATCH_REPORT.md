# PATCH REPORT — Real Hardware Boot Splash Spinner Animation Fix

**Date**: 2026-09-01  
**Git Safety Checkpoint**: `10772dad35d8e636690451e0df7b8ab66e1e0e7d`

---

## 1. Summary of Changes

Fixed the boot splash spinner animation pipeline by restructuring `rook_render_flush()` to execute page rendering before checking dirty bounds, and ensuring continuous invalidation in `boot_page_on_update()`.

---

## 2. Files and Functions Modified

1. [`kernel/shell/rook/src/rook_render.c`](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c)
   - `rook_render_flush()`: Removed premature `if (g_dirty_count == 0) return;` at entry; invoked `current->ops.on_render()` first, followed by dirty rect flushing to physical GOP VRAM.
2. [`kernel/shell/rook/pages/page_boot.c`](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_boot.c)
   - `boot_page_on_update()`: Added `rook_invalidate_full()`.
   - `boot_page_on_render()`: Added `s_canvas_drawn_full` guard for full screen static canvas; restored background only over the 70x60 spinner bounding box on subsequent frames for high performance 60 FPS animation.
3. [`kernel/shell/rook/src/rook_core.c`](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_core.c)
   - `rook_splash_spin()`: Integrated periodic forensic telemetry logging every 15 frames (`[BOOT_ANIM] frame=<N> angle=<ANGLE> render=<N> invalidate=1 present=1`).
