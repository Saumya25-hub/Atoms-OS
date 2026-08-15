# 🔬 FORENSIC INVESTIGATION REPORT: 10-WALLPAPER 1-MINUTE NON-REPEATING ROTATION ENGINE
**Subsystem:** ATOMS OS Wallpaper & Compositor Services (`wallpaper_service.c`, `generate_boot_assets.py`, `page_login.c`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-16  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary & Requirements Analysis
1. **Source:** 10 PNG images (`1.png` through `10.png`) in `D:\Signatures_OS\BOOT-WALLAPPERS`.
2. **Memory Constraint:** Must run cleanly even on low-spec **1 GB RAM** systems (0 heap allocations, maximum 2 static buffers $= 16.5\text{ MB} = 1.6\%$ of 1GB RAM).
3. **Interval & Randomization:**
   - 1-Minute (60-second) automatic interval.
   - Non-repeating random selection (`next_id != current_id`).
4. **Transition:** Smooth 1.0s non-linear cubic cross-fade with 64-bit SIMD blending (0% idle CPU overhead, $<5\mu\text{s}$ cursor safety).

---

## 2. Risk & Architecture Assessment
* **Asset Packaging:** Pre-encode all 10 wallpapers into compact QOI stream arrays (`g_boot_wallpapers_qoi[10]`) via `tools/generate_boot_assets.py`.
* **Zero-Heap In-Place Decoding:** `decode_qoi_to_canvas()` writes directly to static target canvas.
* **Non-Repeating Shuffle:** Guarantees every transition displays a distinct wallpaper.

---

## 3. Files Involved
* `tools/generate_boot_assets.py`: Multi-image QOI compression generator for all 10 wallpapers.
* `kernel/services/wallpaper/boot_assets.h` / `boot_assets.c`: Generated QOI asset tables.
* `kernel/services/wallpaper/wallpaper_service.h` / `wallpaper_service.c`: 60s timer, non-repeating RNG, 1000ms cross-fade engine.
* `kernel/shell/rook/pages/page_login.c`: Hook `wallpaper_service_update(delta_ms)`.
