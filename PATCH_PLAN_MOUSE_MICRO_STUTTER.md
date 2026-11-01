# ATOMS OS — ARCHITECT PATCH PLAN (TASK 2)

**Objective:** Eliminate the 3.5-Second Periodic Screen Refresh and Mouse Movement Stutter on Bare-Metal Hardware.  
**Target Hardware:** ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake Refresh, xHCI)  
**Date:** 2026-11-02  
**Author:** ATOMS OS Architecture Group  
**Rule 0 Compliance:** Architecture & Plan Only (NO CODE MODIFICATIONS)  

---

## 1. What to Modify

### File 1: `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`
1. **Top-Level Page Routing:** Move page checks (`g_hv_dashboard_page == HV_DASHBOARD_PAGE_MOUSE_DEBUG` and `HV_DASHBOARD_PAGE_KEYBOARD_DEBUG`) outside the `if (rt_vm)` guard in `hypervisor_dashboard_render_frame()`.
2. **Periodic Heartbeat Isolation:** In the 50,000-tick loop (`s_dash_ticks % 50000 == 0`), if `g_hv_dashboard_page == HV_DASHBOARD_PAGE_MOUSE_DEBUG`, suppress `hypervisor_dashboard_render_frame()` and only render the spinner/heartbeat. Do not allow full-board hypervisor redraws to clobber the active mouse inspector.

### File 2: `kernel/debug/mouse_debug.c`
1. **Incremental Redraw Protection:** Separate static layout drawing (title bar, column headers, panel borders) from dynamic data drawing (accumulators, packet stream table).
2. Draw static layout once on initial page entry or mode switch, and update only the numerical fields and table rows on incoming packets. This eliminates full-screen PCIe VRAM clears on every packet.

---

## 2. Why This is Required

* **Eliminates Screen Contention:** Prevents the hypervisor stage engine from fighting the mouse inspector for display ownership every 3.5 seconds.
* **Eliminates Polling Starvation:** Eliminates 500ms–1000ms CPU freezes caused by blocking UART COM1 serial writes during periodic stage refreshes, ensuring `xhci_poll()` runs uninterrupted.
* **100% Continuous Fluidity:** Guarantees continuous, buttery-smooth mouse tracking without periodic pauses (`MOVE...................`).

---

## 3. Expected Result

* Real hardware mouse movement will be **100% continuous and fluid** with zero 3–4 second freezes.
* Screen will not flicker or flash hypervisor cards while inspecting mouse telemetry.
* The cursor accumulator will increment in real time with $< 1\text{ms}$ latency.

---

## 4. Risk Analysis & Rollback Plan

* **Risk:** Minimal. Only affects the forensic diagnostic dashboard display paths. The underlying xHCI driver, HRE bytecode parser, and USB stack core are unaffected.
* **Rollback Plan:** Single Git command reverts to certified tag:
  ```powershell
  git checkout v2.1-hre-mouse-stable
  ```
