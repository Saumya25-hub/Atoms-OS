# 📐 ARCHITECTURE PATCH PLAN: RING 3 GUI WINDOW COMPOSITING & EVENT ROUTING
**Subsystem:** BWE Event Pump & Compositor (`bwe_core.c`, `bwe_compositor.c`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-17  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Bridge Ring 0 BWE Compositor with User-owned private window surfaces.
- Bridge Ring 0 BWE Event Pump with the per-window circular event queue (`sys_gui_post_event`).
- Verify end-to-end Ring 3 GUI execution and event delivery in pure UEFI QEMU pre-flight.

---

## 2. Target Files for Modification
1. `kernel/wm/bwe/renderer/bwe_compositor.c`: Add backing buffer pixel composition for `win->control_data.canvas.pixel_buffer`.
2. `kernel/wm/bwe/src/bwe_core.c`: Add event forwarding to `sys_gui_post_event()` for mouse moves, clicks, and keys.

---

## 3. Detailed Logic Changes

### In `kernel/wm/bwe/renderer/bwe_compositor.c`:
Before rendering child controls, check if `win->control_data.canvas.pixel_buffer != NULL`.
Compute client rectangle:
- If not borderless: $cx = x + 5, cy = y + 35, cw = w - 10, ch = h - 40$.
- Clamp against `buffer_w` and `buffer_h`.
- Blit pixel data into `ram_fb->buffer`.

### In `kernel/wm/bwe/src/bwe_core.c`:
In mouse event dispatch:
- Map `bwe_ev.type` (BWE_EVENT_MOUSE_DOWN ➔ BOS_GUI_EVENT_MOUSE_DOWN, BWE_EVENT_MOUSE_UP ➔ BOS_GUI_EVENT_MOUSE_UP, BWE_EVENT_MOUSE_MOVE ➔ BOS_GUI_EVENT_MOUSE_MOVE).
- Compute local window coordinates: $lx = mouse\_x - target\_win->screen\_bounds.x, ly = mouse\_y - target\_win->screen\_bounds.y$.
- Call `sys_gui_post_event(leaf_id, &gui_ev)`.

In keyboard event dispatch:
- Map `bwe_ev.type` (BWE_EVENT_KEY_DOWN ➔ BOS_GUI_EVENT_KEY_DOWN, BWE_EVENT_KEY_UP ➔ BOS_GUI_EVENT_KEY_UP).
- Call `sys_gui_post_event(target_id, &gui_ev)`.

---

## 4. Rollback Plan
Revert changes to Git commit `366bbed`.
