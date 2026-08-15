# 📐 ARCHITECTURE PATCH PLAN: CURSOR FLICKER ELIMINATION & USB HOT-PATH PROFILING
**Subsystem:** ATOMS OS Input & Compositor Subsystems (`xhci.c`, `rook_render.c`, `rook_core.c`, `pointer_velocity.c`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-16  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Remove all `display_print` and UART bottleneck operations from `xhci.c` (`xhci_poll()`).
- Implement atomic backbuffer cursor compositing in `rook_render.c` so the physical GOP VRAM is NEVER written without the cursor (100% flicker-free).
- Implement fast $40\times40$ dirty-rect cursor update on mouse motion for instant sub-millisecond tracking.

---

## 2. Target Files for Modification
1. `kernel/drivers/usb/host/xhci/xhci.c`: Remove `display_print` from `xhci_poll()`.
2. `kernel/shell/rook/src/rook_render.c`: Implement atomic backbuffer compositing in `rook_render_flush()`.
3. `kernel/shell/rook/src/rook_core.c`: Fast dirty-rect trigger in `rook_login_spin()`.

---

## 3. Expected Engineering Results
- **Zero Blinking:** Cursor is atomically composited into the backbuffer before VRAM transfer.
- **Zero Latency:** USB event processing runs in $<1\mu\text{s}$ (no UART holdoff), enabling true 1000Hz hardware performance.

---

## 4. Rollback Plan
Revert changes to Git commit `38cda5e`.
