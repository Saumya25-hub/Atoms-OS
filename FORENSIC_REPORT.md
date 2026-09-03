# FORENSIC REPORT — ACTUAL USB HID KEYBOARD LED BUG INVESTIGATION

**Case ID**: `CASE_20260903_USB_HID_LED`  
**Date**: September 3, 2026  
**Auditor**: Forensic Team  
**Git Baseline**: `cc8c5c43fd9cf8a7012b62aa0d562f13f14458bf`  
**Hardware Platform**: ASUS B750M-K (Intel B760/B750 Chipset, LGA1700) + Intel Core i3-14100F  

---

## 1. Executive Summary & Physical Symptom

On the physical target hardware running ATOMS OS:
1. Keyboard typing works normally (characters are produced, input reports received).
2. Mouse movement and buttons work normally.
3. **The physical Lock LEDs (NumLock, CapsLock, ScrollLock) do NOT synchronize correctly**:
   - NumLock physical LED remains stuck in its initial state.
   - CapsLock physical LED does not toggle when the physical CapsLock key is pressed.
   - The software CapsLock/NumLock flags toggle in memory, but the physical keyboard hardware never reflects the state.

**Correction on Previous Case:**
The prior case (`INPUT_POWER_DEAD_CASE`) was closed as a diagnostic-experiment artifact where `xhci_init()` was intentionally bypassed to test 8042 SMM trapping. With `xhci_init()` running normally (as verified in Test B), the controller is `RUNNING (ACTIVE)`, root-hub ports are in `U0 ACTIVE`, and both peripherals are energized. The real production bug is strictly an **LED synchronization failure in the USB HID keyboard driver path**.

---

## 2. Current Architecture Trace: Physical Keypress to LED Output

A code-level forensic trace was performed through the input pipeline:

### A. Input Report & Keycode Decoding
1. **Physical Keypress**: User presses CapsLock (Usage `0x39`) or NumLock (Usage `0x53`).
2. **xHCI Interrupt IN Transfer**: xHCI receives an 8-byte Boot Keyboard Report: `[modifiers, 0, usage1, usage2, ...]`.
3. **Decoder Entry**: `usb_hid_report_received()` in `kernel/drivers/usb/class/usb_hid.c`:
   - Line 102: Detects `usage_id == 0x39` (CapsLock) $\rightarrow$ executes `s_caps_lock_state = !s_caps_lock_state; g_caps_lock_state = s_caps_lock_state;`
   - Line 107: Detects `usage_id == 0x53` (NumLock) $\rightarrow$ executes `s_num_lock_state = !s_num_lock_state; g_num_lock_state = s_num_lock_state;`
   - Line 112: Detects `usage_id == 0x47` (ScrollLock) $\rightarrow$ executes `g_scroll_lock_state = !g_scroll_lock_state;`
4. **The Missing Transition (ROOT CAUSE 1)**:
   - Immediately following the state toggle, **`usb_hid_set_leds()` is NEVER called**.
   - No USB control transfer (`SET_REPORT`) is queued.
   - No USB interrupt OUT transfer is queued.
   - The driver simply constructs a `KeyboardEvent` and forwards it to `hida_push_keyboard_event()`.

### B. Driver Initialization & One-Time Emission (ROOT CAUSE 2)
1. In `usb_hid_bind()` (`kernel/drivers/usb/class/usb_hid.c` lines 291–294):
   - `usb_hid_set_leds(dev, init_leds);` is executed **exactly once** during initial device configuration.
   - No reference or pointer to `USBDevice*` is stored in static or global storage. Once `usb_hid_bind()` exits, no other subsystem in the kernel has access to the keyboard's `USBDevice*` handle to issue subsequent LED reports.

### C. Subsystem Disconnect in `keyboard.c` (ROOT CAUSE 3)
1. In `kernel/drivers/keyboard/src/keyboard.c` lines 22–26:
   ```c
   void keyboard_sync_leds(void) {
       uint8_t mask = (scroll_lock_on ? 1 : 0) | (num_lock_on ? 2 : 0) | (caps_lock_on ? 4 : 0);
       extern bool ps2_keyboard_set_leds(uint8_t mask);
       ps2_keyboard_set_leds(mask);
   }
   ```
2. Any callers invoking `keyboard_sync_leds()` dispatch exclusively to `ps2_keyboard_set_leds()`, which writes to legacy I/O Port 0x60. Because the hardware target is native UEFI with USB peripherals, this call hits open LPC lines and is swallowed or timed out without ever reaching the USB HID stack.

---

## 3. Bug Class Analysis (Prompt Section 8 Verification)

| Bug Hypothesis | Status | Forensic Finding |
| :--- | :--- | :--- |
| **H1: ATOMS updates software lock state but never sends USB HID Output Report** | **CONFIRMED** | `usb_hid_report_received()` modifies `s_caps_lock_state` and `s_num_lock_state` but never triggers `usb_hid_set_leds()`. |
| **H10: LED state changes internally but never re-sent after init** | **CONFIRMED** | `usb_hid_set_leds()` is called only at line 293 in `usb_hid_bind()` and never again. |
| **H12: ATOMS sends LED state once during init and never again** | **CONFIRMED** | Exactly matches the observed single emission at boot. |
| **H4: Bit mapping mismatch between PS/2 and USB HID** | **CONFIRMED** | PS/2 bitmask uses `Scroll=1, Num=2, Caps=4`. USB HID (Usage Page 0x08) uses `Num=1, Caps=2, Scroll=4`. `keyboard.c` uses PS/2 bitmask, which is incompatible with USB HID. |
| **H7 / H8: Interrupt OUT vs Control SET_REPORT Transport** | **SUSPECTED** | Needs runtime inspection via dedicated forensic panel to confirm whether the keyboard exposes an Interrupt OUT endpoint or accepts EP0 Control `SET_REPORT`. |

---

## 4. Subsystems Involved & Risk Analysis

| Subsystem | File | Role | Risk Assessment |
| :--- | :--- | :--- | :--- |
| **USB HID Class** | `kernel/drivers/usb/class/usb_hid.c` | Manages HID reports, lock states, and `usb_hid_set_leds()` | LOW: Surgical addition of LED dispatch on lock-key press |
| **Keyboard Core** | `kernel/drivers/keyboard/src/keyboard.c` | Generic keyboard state & `keyboard_sync_leds()` | LOW: Bridge `keyboard_sync_leds()` to USB HID backend |
| **xHCI Transfer** | `kernel/drivers/usb/host/xhci/xhci_transfer.c` | Executes control transfers (Setup, Data, Status) | DO NOT TOUCH: Proven functional in Test B |
| **xHCI Init** | `kernel/drivers/usb/host/xhci/xhci.c` | Host controller bringup and port power | DO NOT TOUCH: Stable baseline |
| **Compositor / Queue** | BWE / BCM / HIDA | Event delivery and graphics presentation | SAFE: Completely isolated |

---

## 5. Suspected Fix Outline (No Code)

1. Store the active keyboard's `USBDevice*` reference during `usb_hid_bind()`.
2. Inspect whether the keyboard configuration descriptor includes an Interrupt OUT endpoint or relies on Control `SET_REPORT` (bRequest `0x09`).
3. In `usb_hid_report_received()`, upon detecting a transition of Usage 0x39 (CapsLock) or Usage 0x53 (NumLock), invoke the LED synchronization function with the correct USB HID bitmask (`Num=1, Caps=2, Scroll=4`).
4. Update `keyboard_sync_leds()` in `keyboard.c` to dispatch to the active USB keyboard driver when USB HID is active.
5. Deploy `kernel/debug/usb_hid_led_debug.*` to display live USB keyboard descriptors, transfer latency, xHCI completion codes, and 20-cycle automated test results on screen.
