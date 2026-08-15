# 🛠️ PATCH REPORT: POWER BUTTON ICON ATLAS STRIDE MISMATCH FIX
**Subsystem:** ATOMS OS Rook Shell (`kernel/shell/rook/pages/page_login.c`)  
**Patch Engineer:** Antigravity / ARYA Core Patch Team  
**Date:** 2026-08-16  
**Status:** TASK 3 COMPLETE (Patch Phase)

---

## 1. Files & Functions Changed

### `kernel/shell/rook/pages/page_login.c`
* **Function:** `draw_atlas_icon_centered()` & `page_login_on_render()`
* **Changes:**
  - Added `int icon_size` parameter to `draw_atlas_icon_centered()` to support dynamic icon atlas dimensions.
  - Passed `PWR_ICON_SIZE` (22) for Restart and Shutdown icons to eliminate the 2-pixel stride mismatch.
  - Passed `NATIVE_ICON_SIZE` (24) for standard lock and bottom container icons.
