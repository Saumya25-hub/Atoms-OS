# PATCH_PLAN.md — Unified Dark Canvas & Full Invalidation Architecture Plan

## Executive Summary
This document specifies the plan to unify background canvas styling and force full-frame invalidations on page transitions to eliminate all split-screen color artifacts permanently.

---

## 1. What to Modify

### Modification A: Unified Dark Premium Canvas in wallpaper_service.c
- **File**: [wallpaper_service.c](file:///d:/Signatures_OS/kernel/services/wallpaper/wallpaper_service.c)
- **Plan**:
  1. Change `build_reference_landscape_canvas` to generate a sleek dark premium `#0B0F19` / `#000000` canvas.
  2. Ensures 100% color harmony between Boot Splash and Login Screen.

### Modification B: Full Frame Invalidation on Page Transition
- **File**: [rook_core.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_core.c) & [page_login.c](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_login.c)
- **Plan**:
  1. Add `rook_invalidate_full()` inside `rook_goto(page_id)` when switching pages.
  2. Force `rook_invalidate_full()` during `page_login_on_enter` and `page_login_on_render` transitions.

---

## 2. Expected Result
- **Visual Presentation**: 100% Seamless, flicker-free, artifact-free transition from Boot Splash ➔ Lock Screen ➔ Sign-In Screen ➔ Desktop.
- **Color Consistency**: Pure unified dark premium canvas across 100% of the display.

---

## 3. Rollback Plan
- Revert canvas color function if any asset loading issue arises.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
