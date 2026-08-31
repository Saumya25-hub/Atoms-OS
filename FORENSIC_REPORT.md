# FORENSIC REPORT — ATOMS OS Boot Splash Spinner Animation Investigation

**Date**: 2026-09-01  
**Target Hardware**: Intel Core i3-14100F / i3 4th Gen Haswell LGA1150 (H81 Motherboard), Native UEFI, USB Boot  
**Investigation Focus**: Boot Splash AME Spinner Animation State Update & Dirty Rect Presentation Pipeline  

---

## 1. Forensic Investigation Findings

1. **Spinner Angle / State Update**:
   - `AME_Update(16)` correctly invoked `AME_Spinner_Update()`, which advanced `sp->base_angle` by ~4.11° per frame (~1052 sub-degrees).
2. **Repaint / Present Failure (Root Cause)**:
   - In [`kernel/shell/rook/src/rook_render.c:231`](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c#L231), `rook_render_flush()` checked:
     ```c
     if (!g_gop_fb || g_dirty_count == 0) return;
     ```
   - At the end of Frame 0, `g_dirty_count` was reset to `0` (`rook_render.c:292`).
   - On Frame 1, `rook_render()` called `rook_render_flush()`. Because `g_dirty_count == 0` at the very start of the function, `rook_render_flush()` **returned immediately before invoking `current->ops.on_render()`**.
   - As a result, `boot_page_on_render()` was **never called** for frames 1 through 180!
   - The backbuffer and physical VRAM remained completely frozen on Frame 0, causing the spinner to appear 100% static despite timing pacing functioning properly.

---

## 2. Structural Fixes Applied

1. **Render Pipeline Inversion**:
   - In [`kernel/shell/rook/src/rook_render.c`](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c), moved `current->ops.on_render()` **before** the dirty rect check so that every animation frame is actively rasterized to the backbuffer and registers dirty regions before VRAM transfer.
2. **Explicit Animation Invalidation**:
   - In [`kernel/shell/rook/pages/page_boot.c`](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_boot.c), added `rook_invalidate_full()` in `boot_page_on_update()` and `boot_page_on_enter()`.
   - Optimized `boot_page_on_render()` to draw full canvas on Frame 0 and perform lightweight 19 KB bounding-box background restoration + spinner redrawing on subsequent frames.
3. **Forensic Telemetry**:
   - Added periodic telemetry `[BOOT_ANIM] frame=<N> angle=<ANGLE> render=<N> invalidate=1 present=1` every 15 frames in [`rook_splash_spin()`](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_core.c).
