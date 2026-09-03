# PATCH REPORT — FINAL NATIVE USB HID KEYBOARD LED SYNCHRONIZATION

**Case ID**: `CASE_20260903_USB_HID_LED`  
**Date**: September 3, 2026  
**Patch Team**: Systems Engineering  
**Target Physical Hardware**: ASUS B750M-K (Intel Core i3-14100F, Haswell/RaptorLake UEFI Testbench)

---

## 1. Summary of Changes

1. **`kernel/drivers/usb/class/usb_hid.c`**:
   - Added native HID Report Descriptor parser `hid_parse_keyboard_report_desc()` that walks HID tokens, identifies Usage Page 0x08 (LEDs), Report ID, and bit allocations for Num Lock (0x01), Caps Lock (0x02), and Scroll Lock (0x03).
   - In `usb_hid_bind()`, added discovery of `USBHIDDescriptorHeader`, queried authoritative Report Descriptor from hardware using standard `GET_DESCRIPTOR` (`USB_DESC_HID_REPORT = 0x22`), and parsed the layout.
   - Updated `usb_hid_sync_leds()` and `usb_hid_set_leds()` to dynamically format the Output Report byte using the parsed layout and Report ID.
   - Initialized hardware LEDs at boot (`Num=ON, Caps=OFF, Scroll=OFF`).
   - Dispatched `usb_hid_sync_leds()` immediately on every lock-state transition (Usages `0x39`, `0x53`, `0x47`).

2. **`kernel/drivers/usb/host/xhci/xhci_cmd.c`**:
   - Expanded EP0 transfer ring `g_xhci_ep0_ring[slot_id]` from 64 to 1024 TRBs (341 control transfers per lap), eliminating any ring-wrap bottleneck during high-frequency LED transitions.

3. **`kernel/drivers/usb/host/xhci/xhci.c`**:
   - Expanded event ring `g_xhci_event_ring` to 1024 TRBs and cleared slot 1023 (preventing spurious Link TRB execution on event rings).

4. **`kernel/debug/usb_hid_led_debug.h` & `kernel/debug/usb_hid_led_debug.c`**:
   - Added Scroll Lock 20-cycle benchmark.
   - Tuned settling hold delay to 25ms (40 toggles/second).
   - Updated ABDE dashboard to render authoritative HID Report Descriptor layout metadata and 240/240 transfer metrics.

---

## 2. File and Function Modifications

| File | Functions Modified | Description |
| :--- | :--- | :--- |
| `kernel/drivers/usb/class/usb_hid.c` | `hid_parse_keyboard_report_desc`, `usb_hid_sync_leds`, `usb_hid_set_leds`, `usb_hid_bind` | Native Report Descriptor parser and layout-driven LED synchronization. |
| `kernel/drivers/usb/host/xhci/xhci_cmd.c` | `xhci_cmd_address_device` | Expanded EP0 transfer ring to 1024 TRBs. |
| `kernel/drivers/usb/host/xhci/xhci.c` | `xhci_init` | Expanded event ring to 1024 TRBs and cleared last slot. |
| `kernel/debug/usb_hid_led_debug.h` | `UsbHidLedAudit` | Added `scroll_benchmark` struct. |
| `kernel/debug/usb_hid_led_debug.c` | `audit_run_benchmarks`, `audit_render_dashboard` | Executed 20-cycle Caps, Num, Combined, and Scroll stress suite. |

---

## 3. Preservation of Stable Subsystems

- `push_event()`: **UNTOUCHED**
- Event Queue: **UNTOUCHED**
- Mouse Presenter & Cursor: **UNTOUCHED**
- Keyboard Input Scancode Decoding: **UNTOUCHED**
- Compositor: **UNTOUCHED**
- VMM / PMM / Scheduler: **UNTOUCHED**
- Realtek RTL8168 LAN Datapath: **UNTOUCHED**
