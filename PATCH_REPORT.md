# 🛠️ PATCH REPORT: RING 3 GUI WINDOW COMPOSITING & EVENT ROUTING
**Subsystem:** BWE Event Pump & Compositor (`bwe_core.c`, `bwe_compositor.c`)  
**Patch Engineer:** Antigravity / ARYA Core Patch Team  
**Date:** 2026-08-17  
**Status:** TASK 3 COMPLETE (Patch Phase)

---

## 1. Files & Functions Changed

### 1. `kernel/wm/bwe/renderer/bwe_compositor.c`
* **Function:** `compose_window_recursive()`
* **Changes:**
  - Added blitting of process-owned `win->control_data.canvas.pixel_buffer` into window client bounds `[cx, cy, cw, ch]`.
  - Honors borderless flags and window clipping bounds.

### 2. `kernel/wm/bwe/src/bwe_core.c`
* **Function:** `BWE_PumpEvents()`
* **Changes:**
  - Added translation and forwarding of hardware mouse events (`MOUSE_MOVE`, `MOUSE_DOWN`, `MOUSE_UP`) into `sys_gui_post_event(leaf_id, &gui_ev)`.
  - Added translation and forwarding of hardware keyboard events (`KEY_DOWN`, `KEY_UP`) into `sys_gui_post_event(target_id, &gui_ev)`.
