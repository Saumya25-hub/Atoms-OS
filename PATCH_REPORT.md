# PATCH REPORT — PHYSICAL KEYBOARD INPUT & TELEMETRY
**Subsystem**: Hypervisor Dashboard / xHCI Event Servicing / Live Input Telemetry  
**Target Hardware**: Intel Core i3-14100F (LGA1700), ASUS PRIME B760M-K, Physical USB Keyboard  
**Document**: PATCH_REPORT.md (Task 3 of Engineering Protocol V1)  
**Input References**: FORENSIC_REPORT.md, PATCH_PLAN.md  

---

## 1. Summary of Modifications

Only the file explicitly approved in `PATCH_PLAN.md` was modified:
- `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`

Zero changes made to `kernel.c`, zero changes to `virtio_net.c`, zero changes to `virtio_display.c`, zero changes to `hypervisor.c`.

---

## 2. Detailed Change Audit

### File: `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`

1. **Header & External Declarations** (Lines 24–41):
   - Added `#include "kernel/drivers/usb/core/usb_core.h"`
   - Added `extern void xhci_poll(void);`
   - Added `extern USBDevice* usb_hid_get_keyboard_device(void);`
   - Added live volatile telemetry bindings:
     - `g_xhci_events` (Hardware event ring dequeue counter)
     - `g_xhci_transfers` (Transfer completion event counter)
     - `g_usb_hid_packets` (Received USB HID packet counter)
     - `g_keyboard_events` (Decoded keyboard packet counter)
     - `g_kbd_total_keypresses` (Cumulative physical keypress counter)
     - `g_last_key_usage` (USB HID Usage ID of last pressed key)
     - `g_last_key_mapped` (BOS keycode of last pressed key)
     - `g_last_key_ascii` (Decoded ASCII character of last pressed key)
     - `g_kbd_interface_num` (Assigned keyboard interface number)
     - `g_kbd_ep_addr` (Assigned keyboard interrupt IN endpoint address)
     - `g_irq1_count` (Legacy 8042/PS2 IRQ 1 counter)

2. **Function: `hv_poll_developer_toggle_keys()`** (Lines 1151–1156):
   - Added `xhci_poll();` at function entry before draining `keyboard_poll_event()`.
   - Ensures xHCI hardware event ring is continuously serviced, acknowledging ERDP and passing `TRB_TRANSFER_EVENT` completions to `usb_hid_report_received()` -> `hida_push_keyboard_event()` -> `keyboard_push_event()`.

3. **Function: `hypervisor_dashboard_render_runtime_dashboard()`** (Lines 1403–1491):
   - Added **CARD 6: INPUT HARDWARE & TELEMETRY (STEP 7)** in the left panel under Phase Status.
   - Live telemetry rendered:
     - Controller state & IRQ1 count.
     - Enumerated USB Keyboard device (Slot, VID, PID, IF, EP).
     - Live packet counters: `RX`, `XFER`, `HID`, `KBD`.
     - Last Key Press: Usage ID, BOS Keycode, ASCII character, and total keys read.
   - Updated Bottom Heartbeat Bar to persistently show:
     `Keys: %u (RX:%llu HID:%llu)`

4. **Functions: `hypervisor_dashboard_render_network_debug()` & `hypervisor_dashboard_render_graphics_debug()`** (Lines 1800–1818, 2005–2022):
   - Updated Bottom Heartbeat Bar across F2 (Network) and F3 (Graphics) screens to also display live:
     `Keys: %u (RX:%llu HID:%llu)`
