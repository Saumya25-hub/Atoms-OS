# 📐 ARCHITECTURE PATCH PLAN: 10-WALLPAPER 1-MINUTE NON-REPEATING ROTATION ENGINE
**Subsystem:** ATOMS OS Wallpaper & Compositor Services (`wallpaper_service.c`, `generate_boot_assets.py`, `page_login.c`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-16  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Update `tools/generate_boot_assets.py` to encode all 10 wallpapers from `D:\Signatures_OS\BOOT-WALLAPPERS` into `g_boot_wallpapers_qoi[10]`.
- Implement 60-second non-repeating random wallpaper scheduler in `wallpaper_service.c`.
- Implement smooth 1000ms cubic ease-in-out cross-fade renderer using 2 static 1080p buffers ($16.5\text{ MB}$, 0 bytes heap used).
- Hook `wallpaper_service_update(delta_ms)` into `page_login_on_update()`.

---

## 2. Target Files for Modification
1. `tools/generate_boot_assets.py`
2. `kernel/services/wallpaper/wallpaper_service.h`
3. `kernel/services/wallpaper/wallpaper_service.c`
4. `kernel/shell/rook/pages/page_login.c`

---

## 3. Expected Engineering Results
- **Smooth 1-Min Rotation:** Wallpaper changes automatically every 60 seconds with a 1.0s silky smooth fade.
- **Zero Repetition:** `next_id != current_id` guaranteed.
- **Zero Latency / Zero CPU Strain:** CPU is 0.00% idle outside the 1.0s transition. Cursor remains 100% flicker-free.
- **1GB RAM Compatibility:** Total RAM footprint $\le 16.5\text{ MB}$ (1.6% of 1GB).

---

## 4. Rollback Plan
Revert changes to Git commit `d6a593a`.
