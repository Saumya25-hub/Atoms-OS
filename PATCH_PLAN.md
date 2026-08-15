# 📐 ARCHITECTURE PATCH PLAN: UNIVERSAL MOUSE MULTI-BACKEND ARBITRATION & HYPERVISOR BRIDGE
**Subsystem:** ATOMS OS Input & USB Subsystem (`HIDA`, `VMMouse`, `xHCI`, `PS/2`, `PointerEngine V2`, `ROOK`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-15  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Deliver 100% industrial-grade Real OS standard (Linux `mousedev` / Windows NT `mouclass`) universal mouse input across both physical bare-metal hardware (Intel H81 xHCI USB Mouse & PS/2) and hypervisor virtual machines (VMware Workstation VMMouse, QEMU).
- Eliminate artificial device exclusivity drops in `hida.c`.
- Integrate `vmmouse_poll()` into `rook_login_spin()` supervisor loops.
- Ensure `ccte_push_absolute()` triggers immediate `input_core_dispatch_events()`.

---

## 2. Target Files for Modification
1. `kernel/drivers/input/core/hida.c`:
   - In `hida_push_relative()`, remove the single-owner drop check and ensure all valid incoming USB and PS/2 packets dispatch immediately to `InputCore`.
   - In `hida_push_absolute()`, allow valid absolute packets to route smoothly to `ccte_push_absolute()`.
2. `kernel/drivers/input/core/ccte.c`:
   - In `ccte_push_absolute()`, invoke `input_core_dispatch_events()` after pushing `INPUT_EVENT_TYPE_MOTION_ABSOLUTE`.
3. `kernel/shell/rook/src/rook_core.c`:
   - In `rook_login_spin()`, add `vmmouse_poll()` alongside `xhci_poll()` in both the frame rendering step and the TSC frame pacing wait loop.

---

## 3. Expected Engineering Results
- **Physical H81 Hardware:** USB Mouse movement and clicks are parsed instantly and the cursor moves with subpixel fluid precision.
- **VMware Workstation / VirtualBox:** VMMouse absolute packets are drained continuously on every frame, seamlessly following the host cursor.
- **Latency:** $<0.1\text{ms}$ across all input backends.

---

## 4. Rollback Plan
Revert changes to Git commit `e9e7ca8`.
