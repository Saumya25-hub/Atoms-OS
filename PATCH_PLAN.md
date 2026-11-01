# ATOMS OS — ARCHITECTURE PATCH PLAN (TASK 2)
**Phase:** Phase 1 — Anti-Misrouting Gate & Strict Length Discriminator  
**Input:** `FORENSIC_REPORT.md`  
**Date:** 2026-11-02  
**Author:** ATOMS OS Architecture Group  
**Rule 0 Compliance:** Architecture Specification Only (NO CODE MODIFICATIONS)

---

## 1. WHAT TO MODIFY
Only the following files are authorized for modification in Phase 1:
1. `kernel/drivers/usb/class/usb_hid.c`
2. `kernel/drivers/usb/host/xhci/xhci_transfer.c`

*Strict Prohibition:* `kernel.c`, `kernel/core/*`, `bovisual/*`, `display/*` and all other unrelated files shall NOT be touched.

---

## 2. WHY MODIFY THESE FILES?
1. In `kernel/drivers/usb/class/usb_hid.c`:
   - `usb_hid_report_received()` incorrectly flags reports as keyboard solely because `dev->protocol == 1`, regardless of whether the packet length is 3 bytes (optical mouse) or 8 bytes (boot keyboard).
   - `usb_hid_bind()` assigns `dev->protocol = 1` for the entire physical device during Interface 0 enumeration and subsequently ignores Interface 1 if it is a secondary interface.
2. In `kernel/drivers/usb/host/xhci/xhci_transfer.c`:
   - `xhci_handle_transfer_event()` passes a single hardcoded protocol variable derived from `dev->protocol` instead of evaluating the actual endpoint descriptor or transfer length.

---

## 3. EXACT STRUCTURAL CHANGES PLANNED

### Change A: Strict Packet Signature & Length Discriminator in `usb_hid_report_received()`
- **Logic:**
  - Before checking any `dev->protocol` or `protocol` variable, examine `length`:
    - If `length <= 5` (or if `length == 3 || length == 4 || length == 5 || length == 6` with standard mouse delta format):
      `is_keyboard` is strictly set to `false`.
    - If `length == 8` and `report[1] == 0`:
      `is_keyboard` is strictly set to `true`.
  - Under no circumstance will a packet of length $\le 5$ enter the keyboard keycode translation loop.

### Change B: Isolated Interface Context Binding in `usb_hid_bind()`
- Allow Interface 1 to be registered and bound as `s_usb_mouse_dev` even when Interface 0 was claimed as `s_usb_kbd_dev`.
- Prevent secondary mouse interfaces on composite devices from being dropped.

---

## 4. EXPECTED RESULTS
1. **Immediate Termination of "Mouse Typing Letters":** Mouse movements ($dX, dY$) and clicks will never be passed to `hida_push_keyboard_event()`. No letters or modifiers will be typed when moving or clicking the mouse.
2. **Restoration of Pointer Accumulator:** Mouse packets from Slot 1 Interface 1 on ASUS PRIME B760M-K will route directly to `hida_push_relative(HIDA_BACKEND_USB, dx, dy, hida_buttons, scroll)`.
3. **Telemetry Status:** In the Live Inspector (F8), `Mouse Routed (OK)` will increment from 0 to matching packet counts, and `MISROUTED->KBD!` will drop to 0.

---

## 5. RISK ASSESSMENT
- **Risk Level:** Very Low.
- **Affected Devices:** Only USB devices sending HID reports. PS/2 fallback and VMMouse remain completely untouched.
- **Side Effect Analysis:** Genuine keyboards send 8-byte reports (or occasionally 9-byte reports with report IDs). A check ensuring length $\ge 8$ for keyboards guarantees zero keyboard regression.

---

## 6. ROLLBACK PLAN
If unexpected behavior occurs on real hardware:
1. Revert `kernel/drivers/usb/class/usb_hid.c` via git checkout to checkpoint commit `34139e7`.
2. Revert `kernel/drivers/usb/host/xhci/xhci_transfer.c`.
3. Re-verify build cleanly with `./build.ps1`.

---
**Architectural Approval:** Plan locked. Awaiting user sign-off to proceed to TASK 3 (Patch Execution).
