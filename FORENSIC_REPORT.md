# 🔬 FORENSIC INVESTIGATION REPORT: LOCK SCREEN WALLPAPER PIXELATION & ROTATION SCHEDULER
**Subsystem:** ATOMS OS Wallpaper & Image Generation Systems (`image_builder.c`, `build.ps1`, `generate_boot_assets.py`, `wallpaper_service.c`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-16  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary & Root Cause Analysis
User provided screenshot `Screenshot 2026-08-16 012608.png` showing the Lock Screen photo wallpaper visibly pixelated with blocky artifacts.

### Root Cause 1 (Pixelation): Ultra-Low Resolution Encoding
- In commit `ab0fcb3`, wallpapers were downscaled to $240\times135$ ($8\times8$ pixel block replication) to fit within a legacy $4\text{ MB}$ MBR disk partition cap (`PARTITION_LBA = 8192`).
- On a 1080p panel, $8\times8$ block scaling produces noticeable blockiness and pixelation.
- **The Fix:** Expand `PARTITION_LBA` from `8192` (4MB) to `65536` (32MB) in `image_builder.c` and `$RESERVED_DISK_SECTORS = 65520` in `build.ps1`.
- Encode crystal-clear High-Definition $960\times540$ wallpapers (2x crisp scaling) or full resolution, completely eliminating pixelation.

### Root Cause 2 (Rotation Timer): RTC/TSC Seed Reliability
- `timer_get_ticks()` is not yet initialized during the early login supervisor loop.
- `wallpaper_service_update()` must rely strictly on TSC cycles and frame pacing for deterministic 60-second rotation.

---

## 2. Real OS Standard
* 32MB kernel payload partition alignment for high-fidelity assets.
* High-definition 1080p/540p QOI stream decoder.
* Pure hardware TSC-backed rotation scheduler.

---

## 3. Files Involved
* `tools/image_builder.c`: Update `PARTITION_LBA` to 65536.
* `build.ps1`: Update `$RESERVED_DISK_SECTORS` to 65520.
* `tools/generate_boot_assets.py`: Encode HD $960\times540$ wallpapers.
* `kernel/services/wallpaper/wallpaper_service.c`: Update decoder and TSC rotation triggers with serial logging.
