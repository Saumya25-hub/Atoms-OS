# ARCHITECT PLAN — FINAL NATIVE USB HID KEYBOARD LED SYNCHRONIZATION

**Case ID**: `CASE_20260903_USB_HID_LED`  
**Date**: September 3, 2026  
**Architect**: Architect Team  
**Objective**: Implement native, authoritative USB HID Report Descriptor parsing and robust LED synchronization for Caps Lock, Num Lock, and Scroll Lock.

---

## 1. Architectural Findings & Surgical Scope

1. **Hardware Verification Confirmed**:
   - Physical testbench keyboard: `VID=0xC0F4, PID=0x0201`, Address 4, Slot 4.
   - Endpoint: Interrupt IN on EP 2 (8-byte packet).
   - LED Transport: EP0 Control Transfer `SET_REPORT` (bRequest `0x09`).
2. **Authoritative Report Descriptor**:
   - Query `GET_DESCRIPTOR` for `USB_DESC_HID_REPORT` (`0x22`) using length from `USBHIDDescriptor`.
   - Natively parse Usage Page 0x08 (LEDs), Report ID, and bit positions for Num Lock (0x01), Caps Lock (0x02), and Scroll Lock (0x03).
3. **Queue & Ring Sizing (xHCI Robustness)**:
   - Expand `g_xhci_ep0_ring` from 64 to 1024 TRBs (holds 341 control transfers per lap).
   - Expand `g_xhci_event_ring` to 1024 TRBs and ensure no Link TRB clobbers the event ring.
4. **Input Protection**:
   - `push_event()`, event queue, compositor, mouse presenter, VMM, PMM, scheduler, and network datapath REMAIN 100% UNTOUCHED.

---

## 2. Target Files for Modification

1. `kernel/drivers/usb/class/usb_hid.c`:
   - Add native `hid_parse_keyboard_report_desc()`.
   - Query `GET_DESCRIPTOR` (Report Descriptor) in `usb_hid_bind()`.
   - Implement `usb_hid_sync_leds()` driven by parsed `s_kbd_led_layout`.
   - Ensure initial synchronization (`Num=ON, Caps=OFF, Scroll=OFF`) is emitted at startup.
   - Ensure every lock keypress in `usb_hid_report_received()` triggers `usb_hid_sync_leds()`.
2. `kernel/drivers/usb/host/xhci/xhci_cmd.c`:
   - Allocate 1024 TRBs for `g_xhci_ep0_ring[slot_id]`.
3. `kernel/drivers/usb/host/xhci/xhci.c`:
   - Allocate 1024 TRBs for `g_xhci_event_ring` and clear slot 1023 (pure event ring).
4. `kernel/debug/usb_hid_led_debug.c`:
   - Run 20-cycle Caps, 20-cycle Num, 20-cycle Combined, and 20-cycle Scroll tests with 25ms settling delay.
   - Render updated ABDE dashboard showing report descriptor metadata and 100% pass status.

---

## 3. Rollback Plan

```powershell
git checkout cc8c5c4
```
Restores baseline immediately.
