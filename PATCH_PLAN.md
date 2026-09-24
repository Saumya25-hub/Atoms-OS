# ARCHITECTURE PATCH PLAN — PHYSICAL KEYBOARD INPUT & TELEMETRY
**Subsystem**: Hypervisor Dashboard / xHCI Event Loop / Input Telemetry  
**Target Hardware**: Intel Core i3-14100F (LGA1700), ASUS PRIME B760M-K, Physical USB Keyboard  
**Document**: PATCH_PLAN.md (Task 2 of Engineering Protocol V1)  
**Input Reference**: FORENSIC_REPORT.md  

---

## 1. Scope & Objective
Restore physical USB keyboard input to the ATOMS Phase 5A Runtime Dashboard on the ASUS PRIME B760M-K target without disrupting the running FreeBSD VM or VMX root operation. Expose live, non-synthetic diagnostic telemetry (Step 7) so each stage of the hardware input path is forensically observable on screen.

---

## 2. Files Approved for Modification

| File | Functions / Scope | Rationale |
|---|---|---|
| `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c` | `hv_poll_developer_toggle_keys()`, `hypervisor_dashboard_render_runtime_dashboard()`, `hypervisor_dashboard_render_network_debug()`, `hypervisor_dashboard_render_graphics_debug()` | 1. Service xHCI event ring during input polling.<br>2. Add live non-synthetic input telemetry display (USB_RX, XFER_EVT, HID_RPT, KBD_EVTS, LAST_KEY, Device Info). |
| `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.h` | Header declarations (if any required externs) | Function prototype synchronization. |

**No other files or subsystems are permitted to be touched.**
Specifically:
- `kernel.c`: DO NOT TOUCH.
- `virtio_net.c` / `virtio_display.c`: DO NOT TOUCH.
- `vmx.c` / `hypervisor.c`: DO NOT TOUCH.
- `keyboard.c`: DO NOT TOUCH (core ring buffer logic is verified sound).

---

## 3. Detailed Architectural Modifications

### Modification A: Service xHCI Event Ring in `hv_poll_developer_toggle_keys()`
- **What**: Invoke `xhci_poll()` at the entry of `hv_poll_developer_toggle_keys()` prior to reading `keyboard_poll_event()`.
- **Why**: Since BIOS handoff disables SMM 8042 emulation, USB keyboard reports arrive exclusively as xHCI Interrupt IN Transfer Events. Calling `xhci_poll()` dequeues these events, acknowledges ERDP in hardware, executes `xhci_handle_transfer_event()`, requeues the transfer TRB, invokes `usb_hid_report_received()`, and pushes decoded `KeyboardEvent` structures into `kbd_buffer`.
- **Expected Result**: Pressing any key on the physical USB keyboard will immediately populate `kbd_buffer`, allowing `keyboard_poll_event()` to dequeue the event and increment `g_dashboard_key_press_count`.

### Modification B: Live Real-Time Hardware Input Telemetry Display (Step 7)
- **What**: Render an "INPUT HARDWARE & TELEMETRY" forensic block on the dashboard displaying:
  - `USB Controller` : `xHCI (PCI 0x0C/0x03) [RUNNING]`
  - `USB Keyboard`   : Slot ID, VID:PID, EP Address, Interface Number
  - `Hardware RX`   : `USB_RX` (`g_xhci_events`), `XFER_EVT` (`g_xhci_transfers`)
  - `HID Reports`   : `HID_RPT` (`g_usb_hid_packets`), `KBD_PKTS` (`g_usb_diag.keyboard_packet_count`)
  - `Decoded Events`: `KBD_EVTS` (`g_kbd_total_keypresses`), `IRQ1` (`g_irq1_count`)
  - `Last Key`      : Usage ID, BOS Keycode, ASCII character representation
  - `Keys Consumed` : `g_dashboard_key_press_count`
- **Why**: Allows the engineer to instantly pinpoint which stage of the hardware pipeline advances when a physical key is pressed on bare metal.
- **Expected Result**: Direct visibility into xHCI event delivery, HID packet reception, and dashboard key consumption.

---

## 4. Risk Analysis & Safety Guarantee

- **VM Runtime Isolation**: `xhci_poll()` operates strictly within host physical memory and MMIO registers mapped in the host PML4. It does not touch guest physical memory (GPA), EPT tables, or guest vCPU context. The FreeBSD guest continues executing without interruption.
- **Timing & Overhead**: `xhci_poll()` checks a single in-memory cycle bit and exits immediately (~5ns) when no events are queued. It introduces zero observable jitter to the vCPU loop.
- **No Synthesized Data**: No fake keypresses, no timer-based automatic page toggles, no hardcoded counters. All counters directly bind to hardware/driver volatile variables.

---

## 5. Rollback Plan
If any instability occurs:
1. Revert changes to `hypervisor_dashboard.c` using git checkout:
   `git checkout -- kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`
2. Recompile and verify cleanly.
