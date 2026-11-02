# ATOMS OS — KEYBOARD FORENSIC INVESTIGATION REPORT (TASK 1)
**Subsystem:** USB HID Keyboard Subsystem & Asynchronous LED Synchronization  
**Hardware Target:** ASUS PRIME B760M-K (Intel Core i3-14100F, Haswell H81)  
**Date:** 2026-11-02  
**Author:** ATOMS OS Forensic Engineering Team  
**Compliance:** Rule 0 Phase Isolation (NO CODE MODIFICATIONS IN THIS STEP)

---

## 1. INCIDENT DESCRIPTION & SYMPTOMS

During bare-metal physical testing on real hardware (ASUS PRIME B760M-K, Intel Core i3-14100F), the operator confirmed that mouse movement is operating smoothly without issues (certified in Phase 1-3). However, keyboard operations exhibit the following critical degradations:

1. **NumLock / CapsLock Lag & Freezing:**
   - Toggling NumLock or CapsLock ON works, but toggling OFF is delayed, unresponsive, or requires waiting several seconds.
   - The entire system feels momentarily frozen or stalled when lock keys are toggled.

2. **Function Keys (F2, F3) Require Repeated / Hard Pressing:**
   - In the Hypervisor Dashboard, switching or holding screen views (F2 Network, F3 Graphics) requires pressing the key repeatedly or pressing hard with force ("JOOR SE PRESS KARNA PADTA HE ! TUB JAKE SCREEN HOLD KARNA ESA SUB !").
   - Holding down F2 or F3 does not hold or repeat the view smoothly.

3. **Physical Hardware Forensic Console Telemetry (`uploaded_media_1793571215339.png`):**
   - **Section 1 (Hardware Identification):**
     - `USB Keyboard Status: NOT DETECTED` (Ruby Red)
     - `Vendor ID / Product ID: 0x0000 : 0x0000`
     - `USB Address / Slot ID: Addr=0 Slot=0`
     - `Interrupt IN Endpoint: EP 0 (0B Packet)`
   - **Section 4 (Transaction Audit Log):**
     - All 22 LED synchronization transactions show `Code=1 Status=ERROR`:
       - `#15 NUM_CHANGE OFF ON OFF 0x02 0 1 ERROR`
       - `#16 CAPS_CHANGE OFF OFF OFF 0x00 0 1 ERROR`
       - `#17 CAPS_CHANGE OFF ON OFF 0x02 0 1 ERROR`
       - `#18 NUM_CHANGE ON ON OFF 0x03 0 1 ERROR`
       - `#21 GUEST_FOCUS_RESTORE OFF OFF OFF 0x00 0 1 ERROR`
       - `#22 HOST_FOCUS_RESTORE OFF OFF OFF 0x00 0 1 ERROR`
   - **Section 5 (xHCI Real-Time Telemetry):**
     - `xHCI Transfer Events: 1000`
     - `USB Reports Received: 1000`
     - `Last Event Slot / DCI: Slot=2 DCI=3`
     - `Last Event Code: Code=1`

---

## 2. FORENSIC ROOT CAUSE ANALYSIS

### Root Cause 1: Reentrancy Lockout & 100ms Synchronous EP0 Stall (The Freeze Bug)
* **Code References:** `kernel/drivers/usb/host/xhci/xhci.c:508`, `kernel/drivers/usb/host/xhci/xhci_transfer.c:209-225`, `kernel/drivers/usb/class/usb_hid.c:609-626`.
* **Mechanism:**
  1. An Interrupt IN transfer event arrives on the xHCI event ring from the keyboard (`Slot=2, DCI=3`).
  2. In `xhci_process_events()`, the single-threaded atomic reentrancy guard is acquired:
     ```c
     if (__atomic_test_and_set(&g_xhci_event_processing, __ATOMIC_ACQUIRE)) return;
     ```
  3. `xhci_process_events()` calls `xhci_handle_transfer_event()`, which synchronously calls `usb_hid_report_received_udim()`.
  4. In `usb_hid_report_received_udim()`, a NumLock (Usage 0x53) or CapsLock (Usage 0x39) keypress is detected, which immediately calls `usb_hid_sync_leds_ex()`.
  5. `usb_hid_sync_leds_ex()` calls `usb_control_transfer()`, invoking `xhci_control_transfer()`.
  6. `xhci_control_transfer()` enqueues EP0 Setup, Data, and Status TRBs, rings Doorbell Target 1, and enters a blocking busy-wait loop:
     ```c
     while (!g_xhci_ep0_transfer_complete[slot_id]) {
         xhci_poll(); // Calls xhci_process_events()
         delay_cycles(1000);
         wait++;
         if (wait > 100000) { ... timeout ... }
     }
     ```
  7. **THE DEADLOCK:** `xhci_poll()` invokes `xhci_process_events()`. However, `g_xhci_event_processing` is ALREADY set to 1 by the outer call frame!
  8. `xhci_process_events()` evaluates `__atomic_test_and_set(&g_xhci_event_processing, __ATOMIC_ACQUIRE)`, returns TRUE, and **aborts immediately without processing any event TRBs**.
  9. Therefore, the Transfer Event for EP0 completion can NEVER be consumed from the event ring while inside the wait loop.
  10. The wait loop spins for the full 100,000 iterations (~50ms to 100ms CPU freeze), logs `Control Transfer Timeout!`, marks the transaction with `Status: ERROR`, and resets EP0.
  11. This causes the exact symptoms:
      - Every LED transaction is marked `ERROR` in the audit log.
      - The CPU is frozen for ~100ms on every Num/Caps toggle, blocking subsequent keypresses.

### Root Cause 2: One-Time Static Layout Rendering & NULL Keyboard Device Pointer
* **Code References:** `kernel/debug/usb_hid_led_debug.c:91-105`, `kernel/drivers/usb/class/usb_hid.c:249`.
* **Mechanism:**
  1. `usb_hid_led_debug_run()` executes at early boot and calls `audit_locate_usb_keyboard()` only once during setup.
  2. It relies on the global pointer `s_usb_kbd_dev`. If the keyboard is enumerated on Slot 2 via UDIM slightly after boot init, `s_usb_kbd_dev` was either NULL or unbound during the initial layout render.
  3. `audit_render_static_layout()` prints `NOT DETECTED`, `0x0000 : 0x0000`, `Addr=0 Slot=0` and is never refreshed dynamically.
  4. Meanwhile, Section 5 proves the keyboard is fully active on `Slot=2 DCI=3` (Endpoint 1 IN).

### Root Cause 3: Missing Typematic Repeat (Auto-Repeat) & Key Release (Break) in Host Mode
* **Code References:** `kernel/drivers/usb/class/usb_hid.c:597-604`, `kernel/drivers/keyboard/src/keyboard.c:209-216`.
* **Mechanism:**
  1. In `usb_hid_report_received_udim()`, key reports are checked against `old_report`.
  2. If a key is held down (e.g. F2 or F3), `is_new` evaluates to `false`. The driver logs `Key Usage: 0x3C (HELD)` and **drops the packet entirely without generating an event**.
  3. ATOMS OS does not have a software Typematic Repeat Engine (unlike Linux `input_repeat_key` or Windows `KBDCLASS` DPC timer).
  4. In Host Focus mode, key releases (breaks) are completely ignored; only Make (`pressed = true`) events are dispatched.
  5. If a single Make event occurs while the system is in an LED stall or a heavy 25ms full-frame dashboard redraw, the keystroke is missed.
  6. Holding down the key does NOT generate repeat events, leading the operator to believe the key must be pressed hard or repeatedly.

---

## 3. AFFECTED FILES & FUNCTIONS

1. `kernel/drivers/usb/class/usb_hid.c`:
   - `usb_hid_report_received_udim()`: Directly invokes synchronous LED sync on keypress.
   - `usb_hid_sync_leds_ex()`: Synchronous execution of control transfer.
   - `usb_hid_set_leds_ex()`: Performs blocking control transfer.
   - Key release handling for host mode.
2. `kernel/drivers/usb/host/xhci/xhci_transfer.c`:
   - `xhci_control_transfer()`: Reentrancy collision when called from within `xhci_process_events()`.
3. `kernel/debug/usb_hid_led_debug.c`:
   - `audit_locate_usb_keyboard()`: Static one-time probe, lacks dynamic slot resolution from UDIM.
   - Dynamic update of Section 1 layout when keyboard becomes active.
4. `kernel/drivers/keyboard/src/keyboard.c`:
   - Lacks Typematic Auto-Repeat timer for held keys.
5. `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`:
   - Synchronous redundant full-frame redraw when pressing F2/F3 repeatedly.

---

## 4. SUSPECTED ARCHITECTURAL FIX (CONCEPTUAL ONLY — NO CODE)

1. **ALE (Asynchronous LED Engine):**
   - Decouple LED updates completely from keystroke interrupt processing.
   - When Num/Caps/Scroll lock state changes, update software lock flags immediately (0ns delay) and post a pending mask to an atomic LED mailbox.
   - Execute the actual EP0 control transfer outside the `g_xhci_event_processing` lock (via a dedicated step function `usb_hid_ale_step()` called in cooperative poll/scheduler loops).
   - In `xhci_control_transfer()`, permit EP0 completion draining without reentrancy deadlock.

2. **UDIM Dynamic Keyboard Binding:**
   - Enhance `usb_hid_get_keyboard_device()` to query the authoritative UDIM endpoint registry (`g_udim_endpoints[slot][dci]`) if `s_usb_kbd_dev` is not yet bound.
   - In `usb_hid_led_debug.c`, dynamically refresh Section 1 identity when a keyboard is active.

3. **TRE (Typematic Repeat Engine):**
   - Implement an autonomous software key repeat engine (250ms initial delay, 30Hz repeat rate).
   - Dispatch `pressed = false` (Break) events in host mode when a key is released.
   - Releasing a key immediately cancels the repeat timer.

4. **Dashboard Redraw Guard:**
   - In `hypervisor_dashboard.c`, only trigger `hypervisor_dashboard_render_frame()` when the active page index actually changes.

---

## 5. RISK ANALYSIS

- **Risk 1:** Breaking mouse stability achieved in Phase 1-3.
  - *Mitigation:* Touch ONLY keyboard endpoints and LED transport. The mouse packet path (`ep_type == UDIM_DEV_MOUSE`), HRE parser, and cursor accumulator will remain completely untouched.
- **Risk 2:** Regressing PS/2 keyboard support.
  - *Mitigation:* PS/2 keyboard routes through `keyboard_irq_handler` / `i8042`. ALE applies exclusively to USB HID.

---
**Verdict:** Forensic investigation complete. Proceed to TASK 2 (Architecture & Patch Plan).
