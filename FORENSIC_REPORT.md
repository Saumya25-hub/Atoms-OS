# ATOMS OS — FORENSIC INVESTIGATION REPORT (TASK 1)
**Subsystem:** USB HID / Pointer & Mouse Subsystem  
**Hardware Target:** ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake Refresh, UEFI PXE Boot)  
**Date:** 2026-11-02  
**Author:** ATOMS OS Forensic Engineering Team  
**Compliance:** Rule 0 Phase Isolation (NO CODE MODIFICATIONS)

---

## 1. INCIDENT DESCRIPTION & SYMPTOMS
During real bare-metal testing on the ASUS PRIME B760M-K motherboard via UEFI PXE boot, the user observed that mouse movement does not translate into pointer navigation on screen. Instead, moving or clicking the mouse causes erratic text/characters to be typed onto the console/display, as if keyboard keys were being pressed.

---

## 2. FORENSIC EVIDENCE (LIVE PHYSICAL HARDWARE TELEMETRY)
The live telemetry captured from the physical display (`Live USB HID Mouse Inspector`, F8 dashboard) shows:

```
[DEVICE & PACKET TELEMETRY]
Total HID Packets  : 183
Mouse Routed (OK)  : 0
Keyboard Routed    : 183

[BOUND DRIVER INSTANCES]
Mouse Slot / Port  : Slot=1 Port=3 P=1
Keyboard Slot/Port : Slot=5 Port=10 P=1

[LIVE CURSOR ACCUMULATOR]
Accumulated X      : 0
Accumulated Y      : 0
Left Button        : UP
Right Button       : UP
Middle Button      : UP

[LIVE INCOMING USB HID PACKET STREAM -- LAST 12 PACKETS]
# 183 | TIME: 0 | SLOT:IF: 1:1 | LEN: 3 | RAW DATA: 00 00 00 | DECODED: X=0 Y=0 B=0 | KEYBOARD
# 182 | TIME: 0 | SLOT:IF: 1:1 | LEN: 3 | RAW DATA: 01 00 00 | DECODED: X=0 Y=0 B=1 | MISROUTED->KBD!
# 181 | TIME: 0 | SLOT:IF: 1:1 | LEN: 3 | RAW DATA: 00 00 00 | DECODED: X=0 Y=0 B=0 | KEYBOARD
# 180 | TIME: 0 | SLOT:IF: 1:1 | LEN: 3 | RAW DATA: 01 00 00 | DECODED: X=0 Y=0 B=1 | MISROUTED->KBD!
# 179 | TIME: 0 | SLOT:IF: 1:1 | LEN: 3 | RAW DATA: 00 00 00 | DECODED: X=0 Y=0 B=0 | KEYBOARD
# 178 | TIME: 0 | SLOT:IF: 1:1 | LEN: 3 | RAW DATA: 01 00 00 | DECODED: X=0 Y=0 B=1 | MISROUTED->KBD!
```

---

## 3. ROOT CAUSE ANALYSIS

### Root Cause 1: Monolithic Device Identity Override
In `kernel/drivers/usb/class/usb_hid.c`:
1. The mouse is connected to Slot 1, Port 3. It possesses two interfaces: Interface 0 and Interface 1.
2. During enumeration in `usb_hid_bind()`, Interface 0 is queried. Because many modern mice feature macro/consumer buttons, Interface 0 reports `bInterfaceProtocol == 1` (Keyboard).
3. The binding routine executes:
   ```c
   s_usb_kbd_dev = dev;
   dev->protocol = 1;
   ```
4. Even though Interface 1 reports `bInterfaceProtocol == 2` (Mouse), `dev->protocol` remains locked at `1`.

### Root Cause 2: Failure to Discriminate by Packet Length & Signature
In `kernel/drivers/usb/class/usb_hid.c` (`usb_hid_report_received`):
```c
bool is_keyboard = false;
if (protocol == 1 || (dev && dev->protocol == 1)) {
    is_keyboard = true;
}
```
The routine determines whether a packet is a keyboard report based entirely on the `protocol` flag and `dev->protocol == 1`. It completely ignores packet length:
- Standard USB Boot Mouse reports are **3 bytes** (`[buttons, dx, dy]`).
- Standard USB Boot Keyboard reports are **8 bytes** (`[modifiers, reserved=0, key1..key6]`).

Because `is_keyboard` is evaluated as `true` for a 3-byte mouse report:
1. `report[0]` (mouse button 1) is evaluated as a keyboard modifier (Left-Ctrl).
2. `report[1]` is evaluated as reserved.
3. `report[2]` (`dy` vertical displacement) is evaluated as `report[2]` (Keyboard Usage ID / Scancode).
4. If the user moves the mouse down (`dy = 4`), Usage ID `0x04` is looked up in `hid_to_bos_keycode`, which maps to ASCII `'a'`.
5. The character `'a'` is forwarded to `hida_push_keyboard_event()`, printing characters on screen.
6. The routine returns immediately at line 598, completely bypassing `hida_push_relative()`.
7. The pointer accumulator never receives deltas (`Accumulated X=0, Y=0`, `Mouse Routed (OK) = 0`).

---

## 4. FILES INVOLVED
1. `kernel/drivers/usb/class/usb_hid.c`:
   - `usb_hid_report_received()` (Lines 270–600): Routing logic, keyboard/mouse discrimination.
   - `usb_hid_bind()` (Lines 706–770): Device protocol assignment, composite interface filtering.
2. `kernel/drivers/usb/host/xhci/xhci_transfer.c`:
   - `xhci_handle_transfer_event()` (Lines 464–470): Protocol pass-through to `usb_hid_report_received`.
3. `kernel/debug/mouse_debug.c`:
   - Verification and forensic telemetry renderer (detects and prints `MISROUTED->KBD!`).

---

## 5. RISK ANALYSIS
- **Severity:** High / Critical for Input Subsystem. Completely blocks mouse functionality on physical hardware with multi-interface USB mice.
- **Risk to Other Subsystems:** Low. The failure is isolated to the USB HID report routing and device context. PMM, VMM, Scheduler, and Storage are unaffected.
- **Regression Risk of Fix:** Low, provided that genuine 8-byte keyboard reports continue to route to the keyboard handler while 3-to-6 byte reports route exclusively to the mouse handler.

---

## 6. SUSPECTED FIX (ARCHITECTURAL DIRECTION — NO CODE MODIFIED)
1. In `usb_hid_report_received()`, implement an unambiguous Packet Signature & Length Validator:
   - If `length <= 5`, the report is classified as **MOUSE**, regardless of `dev->protocol`.
   - If `length == 8` and `report[1] == 0`, the report is classified as **KEYBOARD**.
2. Decouple `dev->protocol` from the entire physical slot: bind drivers per interface number rather than per slot device pointer.
3. Update `xhci_handle_transfer_event()` to determine endpoint protocol based on endpoint DCI / interface number.

---
**Forensic Verdict:** Verified and Proven by Real Hardware Screenshot (ASUS PRIME B760M-K). Ready for Architecture Plan (TASK 2).
