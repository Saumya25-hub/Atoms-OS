# 📐 ARCHITECTURE PATCH PLAN: REAL OS MOUSE SUBSYSTEM & RELATIVE MOTION PIPELINE
**Subsystem:** ATOMS OS Input & USB Subsystem (`xHCI`, `USB HID`, `InputCore`, `PointerEngine V2`, `ROOK`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-15  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Establish a direct, lockless, zero-latency Linux `libinput` / Windows NT style mouse pipeline from USB HID and PS/2 drivers to `Pointer Engine V2`.
- Eliminate virtual 16-bit coordinate quantization in relative mouse events and process true subpixel relative motion.
- Ensure the input event queue is pumped continuously in `rook_login_spin()` and `rook_update()`.
- Center the cursor initially at $(960, 540)$ on 1080p display with bounds $(1920, 1080)$.

---

## 2. Target Files for Modification
1. `kernel/drivers/input/core/hida.c`:
   - In `hida_push_relative()`, construct an `INPUT_EVENT_TYPE_MOTION_RELATIVE` event directly and dispatch via `input_core_push_event()` and `input_core_dispatch_events()`.
2. `kernel/shell/rook/src/rook_core.c`:
   - In `rook_login_spin()`, add `input_core_dispatch_events()` alongside `xhci_poll()` in both the frame loop and the TSC pacing wait loop.
3. `kernel/drivers/input/pointer/pointer_state.c`:
   - In `pointer_state_init()`, initialize default position to center of screen $(960, 540)$ if $x=0, y=0$.
4. `kernel/drivers/input/input.c`:
   - In `kernel_input_update_resolution()`, call `pointer_bounds_update(w, h)` to synchronize 1920x1080 bounds.

---

## 3. Expected Engineering Results
- **Movement:** Moving the USB mouse on physical hardware immediately moves the cursor arrow across the 1080p screen with zero lag and subpixel precision.
- **Buttons:** Left click, Right click, and Middle click are processed with exact tri-state resolution.
- **Latency:** $<0.1\text{ms}$ latency from xHCI TRB reception to cursor coordinate update.

---

## 4. Rollback Plan
If any input issue occurs, revert changes to Git commit `748bab8`.
