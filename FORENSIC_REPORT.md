# 🔬 FORENSIC INVESTIGATION REPORT: POWER BUTTON ICON ATLAS STRIDE MISMATCH
**Subsystem:** ATOMS OS Rook Shell (`kernel/shell/rook/pages/page_login.c`, `clock_atlas.h`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-16  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary & Forensic Evidence
User provided screenshot `Screenshot 2026-08-16 005310.png` showing the Restart (`↻`) and Shutdown (`⏻`) button icons sheared with horizontal interlaced scanlines and visual distortion.

### Root Cause Analysis:
* `g_restart_icon_atlas` and `g_shutdown_icon_atlas` are statically compiled as `PWR_ICON_SIZE * PWR_ICON_SIZE` ($22\times22$ bytes) in `clock_atlas.h`.
* `draw_atlas_icon_centered()` hardcoded `size = NATIVE_ICON_SIZE` ($24\times24$ bytes).
* When reading the $22\times22$ array using stride 24 (`y * 24 + x`), every row was offset by 2 bytes ($24 - 22 = 2$).
* This 2-pixel cumulative row stride mismatch produced the diagonal shearing and horizontal scanline artifacts visible in the screenshot.

---

## 2. Real OS Standard
* Generalized `draw_atlas_icon_centered()` to accept dynamic `icon_size` ($24$ for standard atlas icons, $22$ for power controls).

---

## 3. Files Involved
* `kernel/shell/rook/pages/page_login.c`: Update `draw_atlas_icon_centered()` signature and pass `PWR_ICON_SIZE` (22) for power controls and `NATIVE_ICON_SIZE` (24) for standard icons.
