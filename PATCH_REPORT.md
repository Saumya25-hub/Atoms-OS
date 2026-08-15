# 🛠️ PATCH REPORT: HD WALLPAPERS & 1-MINUTE ROTATION FIX
**Subsystem:** ATOMS OS Wallpaper & Image Generation Systems (`image_builder.c`, `build.ps1`, `generate_boot_assets.py`, `wallpaper_service.c`)  
**Patch Engineer:** Antigravity / ARYA Core Patch Team  
**Date:** 2026-08-16  
**Status:** TASK 3 COMPLETE (Patch Phase)

---

## 1. Files & Functions Changed

### 1. `tools/image_builder.c`
* **Changes:**
  - Expanded `PARTITION_LBA` from `8192` (4MB) to `65536` (32MB) to support full high-definition embedded assets.

### 2. `build.ps1`
* **Changes:**
  - Expanded `$RESERVED_DISK_SECTORS` from `8180` to `65520` (32MB payload area).

### 3. `tools/generate_boot_assets.py`
* **Changes:**
  - Re-encoded all 10 wallpapers in High-Definition $960\times540$ QOI format for 100% crystal-clear 1080p scaling without blocky pixelation.

### 4. `kernel/services/wallpaper/wallpaper_service.c`
* **Functions:** `decode_qoi_to_canvas()`, `wallpaper_service_select_random()`, `wallpaper_service_update()`
* **Changes:**
  - Added direct 2x integer scaler for $960\times540$ QOI wallpapers to dense 1080p canvas.
  - Upgraded non-repeating RNG to hardware TSC 64-bit LCG.
  - Added COM1 serial diagnostic logging on every 1-minute wallpaper transition.
