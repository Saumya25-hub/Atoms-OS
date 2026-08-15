# 📐 ARCHITECTURE PATCH PLAN: ZERO-LATENCY HARDWARE CURSOR PLANE & XHCI IMOD OPTIMIZATION
**Subsystem:** ATOMS OS Compositor (`ROOK`, `rook_render.c`, `rook_core.c`, `xhci.c`, `pointer_velocity.c`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-16  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Implement a dedicated `rook_cursor_micro_blit()` Save-Behind restoration engine in `rook_render.c`.
- Decouple cursor movement from 60Hz widget re-rendering to achieve $<3\mu\text{s}$ scanout latency.
- Set `*imod = 0` in `xhci.c` to eliminate USB interrupt throttling on Intel Haswell xHCI.
- Integrate `rook_cursor_micro_blit()` into `rook_login_spin()`.

---

## 2. Target Files for Modification
1. `kernel/shell/rook/src/rook_render.c`: Implement `rook_cursor_micro_blit()`.
2. `kernel/shell/rook/src/rook_core.c`: Call `rook_cursor_micro_blit()` during sub-ms hardware polling.
3. `kernel/drivers/usb/host/xhci/xhci.c`: Write `*imod = 0` during interrupter 0 initialization.

---

## 3. Expected Engineering Results
- **Cursor Scanout Latency:** Reduced from $5\text{ms}$–$8\text{ms}$ down to $<0.003\text{ms}$ ($3\mu\text{s}$).
- **Polling Responsiveness:** Instant 1000Hz butter motion parity with Windows 11 DWM / macOS WindowServer.

---

## 4. Rollback Plan
Revert changes to Git commit `1d26069`.
