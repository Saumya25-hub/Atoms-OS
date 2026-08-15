# 🔬 FORENSIC INVESTIGATION REPORT: REAL OS MOUSE SUBSYSTEM & RELATIVE MOTION PIPELINE
**Subsystem:** ATOMS OS Input & USB Subsystem (`xHCI`, `USB HID`, `InputCore`, `PointerEngine V2`, `ROOK`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-15  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary
On physical H81 hardware, the mouse pointer arrow is rendered on screen (thanks to the ARYA Compositor Hook), but remains static at $(0, 0)$ top-left corner because:
1. The `InputCore` event queue (`g_core_queue`) was never drained/dispatched in the main supervisor execution loops.
2. `ccte_push_relative()` was converting raw relative displacement into an artificial 16-bit virtual space (`CCTE_VIRTUAL_MAX = 65535`), resulting in coordinate quantization loss and deadlock.
3. `PointerState` was uninitialized at $(0, 0)$ instead of screen center $(960, 540)$ at boot.

---

## 2. Real OS Comparative Analysis (Windows NT `mouclass` / Linux `libinput`)
* **Linux `libinput` Standard:** Relative mouse packets from USB HID (`dx`, `dy`, `buttons`) are immediately passed as `EV_REL` events directly into the pointer ballistics engine (`libinput_pointer_notify_motion`).
* **Windows NT `win32k` Standard:** Mouse interrupts push `MOUSE_INPUT_DATA` into the raw input thread, which immediately applies subpixel acceleration and updates the global `gpsi->ptCursor`.
* **ATOMS OS Failure Mode:** Mouse events were pushed to a queue that was never pumped by `input_core_dispatch_events()`, leaving `PointerState` frozen at $(0, 0)$.

---

## 3. Files Involved
1. `kernel/drivers/input/core/hida.c`: `hida_push_relative()`
2. `kernel/drivers/input/core/input_core.c`: `input_core_dispatch_events()`
3. `kernel/drivers/input/pointer/pointer_state.c`: `pointer_state_init()`
4. `kernel/drivers/input/input.c`: `kernel_input_update_resolution()`
5. `kernel/shell/rook/src/rook_core.c`: `rook_login_spin()` supervisor pump

---

## 4. Suspected Fix
- Directly dispatch `INPUT_EVENT_TYPE_MOTION_RELATIVE` events in `hida_push_relative()` to `input_core_push_event()` and invoke `input_core_dispatch_events()`.
- Add active `input_core_dispatch_events()` pump in `rook_login_spin()` alongside `xhci_poll()`.
- Initialize `PointerState` and `pointer_bounds` to center $(960, 540)$ with full 1920x1080 bounds.
