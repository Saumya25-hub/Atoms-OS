# 🔬 FORENSIC INVESTIGATION REPORT: UNIVERSAL MOUSE MULTI-BACKEND ARBITRATION & HYPERVISOR BRIDGE
**Subsystem:** ATOMS OS Input & USB Subsystem (`HIDA`, `VMMouse`, `xHCI`, `PS/2`, `PointerEngine V2`, `ROOK`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-15  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary
Cross-platform hardware testing on physical Intel Haswell H81 motherboard and VMware Workstation revealed that mouse cursor remained frozen at $(0, 0)$ / screen edge because:
1. **Real Hardware (xHCI USB Mouse):** `hida.c` contained an artificial owner exclusivity check (`g_hida_owner != backend_id`) which permanently dropped incoming USB mouse packets (backend 121) when VMMouse/PS2 (backend 120/122) was registered at boot.
2. **VMware Workstation (VMMouse):** `vmmouse_poll()` was missing in the `rook_login_spin()` supervisor loop, leaving hypervisor backdoor packets undrained.
3. **CCTE Absolute Ingest:** `ccte_push_absolute()` did not invoke `input_core_dispatch_events()`, leaving absolute packets queued.

---

## 2. Real OS Comparative Analysis (Windows NT `mouclass` / Linux `evdev`)
* **Linux `evdev` / `mousedev` Multi-Device Standard:** The Linux input subsystem maintains an open aggregator (`/dev/input/mice`). When multiple pointing devices (USB mouse, PS/2 mouse, I2C touchpad, VirtIO/VMMouse) are connected, events from any device are multiplexed into the cursor core without dropping valid packets.
* **Windows NT `mouclass.sys` Standard:** Windows NT binds all detected mouse class devices to `\Device\PointerClass0..N`. The Raw Input Thread (RIT) drains all device queues concurrently.
* **ATOMS OS Failure Mode:** `HIDA` attempted an exclusive single-owner lock that permanently suppressed USB packets on real hardware and omitted hypervisor polling in ROOK.

---

## 3. Files Involved
1. `kernel/drivers/input/core/hida.c`: Remove artificial exclusivity drop in `hida_push_relative()` and `hida_push_absolute()`.
2. `kernel/drivers/input/core/ccte.c`: Call `input_core_dispatch_events()` in `ccte_push_absolute()`.
3. `kernel/shell/rook/src/rook_core.c`: Add `vmmouse_poll()` in `rook_login_spin()` alongside `xhci_poll()`.

---

## 4. Suspected Fix
- Enable universal multi-device multiplexing in `hida.c` (Linux `mousedev` standard).
- Add `vmmouse_poll()` to supervisor and frame pacing loops in `rook_core.c`.
- Ensure instant $<0.1\text{ms}$ subpixel dispatch across all backends.
