# 📐 ARCHITECTURE PATCH PLAN: COMPOSITOR CURSOR DECOUPLING & HIGH-DPI KINEMATIC TUNING
**Subsystem:** ATOMS OS Input & Compositor Engine (`ROOK`, `PointerEngine`, `pointer_velocity`, `DGL`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-15  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Deliver zero-latency ($<1\text{ms}$), 1000Hz-responsive cursor motion conforming strictly to Windows DWM / macOS WindowServer hardware plane emulation standards.
- Decouple mouse cursor refresh from the 60Hz scene graph render loop.
- Apply calibrated 1080p kinematic ballistic acceleration.

---

## 2. Target Files for Modification
1. `kernel/shell/rook/src/rook_core.c`:
   - In `rook_login_spin()`, check for pointer coordinate changes during the sub-ms polling loop and immediately trigger direct cursor dirty rect scanout via `rook_render_flush()`.
2. `kernel/drivers/input/pointer/pointer_velocity.c`:
   - In `pointer_velocity_init()`, configure base sensitivity to $1.35\times$ ($88473$ FP16), threshold to $35\text{ px/sec}$, gain to $0.70\times$ ($45875$ FP16), and max clamp to $3.8\times$ ($249036$ FP16).

---

## 3. Expected Engineering Results
- **Cursor Response Latency:** Reduced from $16.6\text{ms}$ to $<0.1\text{ms}$ ($\approx 10\mu\text{s}$).
- **Motion Feel:** Silky smooth, instantaneous butter glide matching native macOS / Windows 11 desktop experience.

---

## 4. Rollback Plan
Revert changes to Git commit `2af7d4e`.
