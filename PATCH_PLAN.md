# 📐 ARCHITECTURE PATCH PLAN: POWER BUTTON ICON ATLAS STRIDE MISMATCH
**Subsystem:** ATOMS OS Rook Shell (`kernel/shell/rook/pages/page_login.c`, `clock_atlas.h`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-16  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Update `draw_atlas_icon_centered()` to accept explicit `int icon_size` parameter.
- Pass `PWR_ICON_SIZE` (22) when rendering `g_restart_icon_atlas` and `g_shutdown_icon_atlas`.
- Pass `NATIVE_ICON_SIZE` (24) when rendering `g_lock_icon_atlas`, `g_ethernet_icon_atlas`, and `g_chat_icon_atlas`.

---

## 2. Target Files for Modification
1. `kernel/shell/rook/pages/page_login.c`:
   - Update `draw_atlas_icon_centered()` definition and all call sites.

---

## 3. Expected Engineering Results
- **Icon Quality:** 100% crisp, razor-sharp 1:1 pixel rendering for Restart and Shutdown icons with zero horizontal scanlines, shearing, or stride distortion.

---

## 4. Rollback Plan
Revert changes to Git commit `b049aa4`.
